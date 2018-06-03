#include <iostream>
#include <experimental/filesystem>

#include <grpc++/server_context.h>

#include "chord.log.h"
#include "chord.uri.h"
#include "chord.context.h"
#include "chord.router.h"

#include "chord.common.h"
#include "chord.client.h"
#include "chord.service.h"


using grpc::ServerContext;
using grpc::ClientContext;
using grpc::ServerBuilder;
using grpc::ServerReader;
using grpc::Status;

using chord::common::Header;
using chord::JoinResponse;
using chord::JoinRequest;
using chord::SuccessorResponse;
using chord::SuccessorRequest;
using chord::StabilizeResponse;
using chord::StabilizeRequest;
using chord::NotifyResponse;
using chord::NotifyRequest;
using chord::CheckResponse;
using chord::CheckRequest;
using chord::Chord;


namespace fs = std::experimental::filesystem::v1;
using namespace std;
using namespace chord::common;

namespace chord {

Service::Service(Context &context, Router *router)
    : context{context},
      router{router},
      make_client{
          [this] { return chord::Client(this->context, this->router); }},
      logger{log::get_or_create(logger_name)} {}

Service::Service(Context &context, Router *router, ClientFactory make_client)
    : context{context}, router{router}, make_client{make_client}, logger{log::get_or_create(logger_name)} {}

Status Service::join(ServerContext *serverContext, const JoinRequest *req, JoinResponse *res) {
  (void)serverContext;
  auto id = req->header().src().uuid();
  auto endpoint = req->header().src().endpoint();

  logger->trace("join request from {}@{}", id, endpoint);
  /**
   * find successor
   */
  auto src = uuid_t{id};
  auto entry = successor(src);

  res->mutable_successor()->CopyFrom(entry);

  /**
   * initialize the ring
   */
  if (router->successor(0)==nullptr) {
    logger->info("first node joining, setting the node as successor");
    router->set_successor(0, src, endpoint);
  }

  return Status::OK;
}

RouterEntry Service::successor(const uuid_t &uuid) {
  ServerContext serverContext;
  SuccessorRequest req;
  SuccessorResponse res;

  req.mutable_header()->CopyFrom(make_header(context));
  req.set_id(uuid);

  Status status = successor(&serverContext, &req, &res);

  if (!status.ok()) throw__grpc_exception("failed to query successor", status);

  return res.successor();
}

Status Service::successor(ServerContext *serverContext, const SuccessorRequest *req, SuccessorResponse *res) {
  (void)serverContext;
  logger->trace("successor from {}@{} successor of? {}",
                req->header().src().uuid(), req->header().src().endpoint(),
                req->id());

  //--- destination of the request
  uuid_t id(req->id());
  uuid_t self(context.uuid());
  uuid_t successor(*router->successor());

  logger->trace("[successor] id {}, self {}, successor{}", id, self, successor);

  if (id > self and (id <= successor or (successor < self and id > successor))) {
    logger->trace("successor of {} is {}", id, successor);
    //--- router entry
    RouterEntry entry;
    entry.set_uuid(successor);
    entry.set_endpoint(router->get(successor));

    res->mutable_successor()->CopyFrom(entry);
  } else {
    logger->trace("[successor] id not within self ({}) <--> successor({}) trying to forward.", context.uuid() 
                                  , *router->successor());
    uuid_t next = router->closest_preceding_node(id);
    logger->trace("[successor] closest preceding node {}", next);
    if (next==self) {
      RouterEntry entry;
      entry.set_uuid(self);
      entry.set_endpoint(router->get(self));

      res->mutable_successor()->CopyFrom(entry);
    } else {
      logger->trace("[successor] trying to spawn client to forward request.");
      Status status = make_client().successor(req, res);
      if (!status.ok()) {
        logger->warn("[successor] failed to query successor from client!");
      } else {
        logger->trace("spawned client, got result");
        logger->trace("entry {}@{}", res->successor().uuid(), res->successor().endpoint());
      }
    }
  }

  return Status::OK;
}

Status Service::stabilize(ServerContext *serverContext, const StabilizeRequest *req, StabilizeResponse *res) {
  (void)serverContext;
  logger->trace("stabilize from {}@{}", req->header().src().uuid(),
                req->header().src().endpoint());

  auto predecessor = router->predecessor();
  if (predecessor!=nullptr) {
    logger->debug("returning predecessor {}", *predecessor);
    endpoint_t endpoint = router->get(predecessor);

    RouterEntry entry;
    entry.set_uuid(*predecessor);
    entry.set_endpoint(endpoint);

    res->mutable_predecessor()->CopyFrom(entry);
  }

  return Status::OK;
}

Status Service::notify(ServerContext *serverContext, const NotifyRequest *req, NotifyResponse *res) {
  (void)serverContext;
  (void)res;
  logger->trace("notify from {}@{}", req->header().src().uuid(),
                req->header().src().endpoint());

  auto predecessor = router->predecessor();

  //--- validate
  if (!req->has_header() && !req->header().has_src()) {
    logger->warn("failed to validate header");
    return Status::CANCELLED;
  }

  uuid_t self(context.uuid());
  uuid_t uuid = uuid_t(req->header().src().uuid());
  endpoint_t endpoint = req->header().src().endpoint();

  if (predecessor==nullptr or
      (uuid > *predecessor and uuid < self) or
      (uuid > *predecessor and uuid > self)) {
    router->set_predecessor(0, uuid, endpoint);
  } else {
    if (predecessor==nullptr)
      logger->info("predecessor is null");
    else {
      logger->info("n' = {}\npredecessor = {}\nself={}", uuid, *predecessor, self);
    }
  }

  return Status::OK;
}

Status Service::check(ServerContext *serverContext, const CheckRequest *req, CheckResponse *res) {
  (void)serverContext;
  logger->trace("check from {}@{}", req->header().src().uuid(),
                req->header().src().endpoint());
  res->mutable_header()->CopyFrom(make_header(context));
  res->set_id(req->id());
  return Status::OK;
}

void Service::fix_fingers(size_t index) {
  uuid_t fix = context.uuid();
  if (router->successor(0)!=nullptr) {
    fix += uuid_t(pow(2., (double) index - 1));
    logger->trace("fix_fingers router successor is not null, increasing uuid");
  }

  logger->trace("fixing finger for {}.", fix);

  try {
    auto entry = successor(fix);
    logger->trace("fixing finger for {}. received successor {}@{}", fix,
                  entry.uuid(), entry.endpoint());
    //if( uuid_t(router_entry.uuid()) == context.uuid() ) return;

    router->set_successor(index, uuid_t{entry.uuid()}, entry.endpoint());
  } catch (const chord::exception &e) {
    logger->warn("failed to fix fingers for {}", fix);
  }
}

} //namespace chord
