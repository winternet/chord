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
  const auto id = req->header().src().uuid();
  const auto endpoint = req->header().src().endpoint();

  logger->trace("join request from {}@{}", id, endpoint);
  /**
   * find successor
   */
  const auto src = uuid_t{id};
  const auto entry = successor(src);

  res->mutable_successor()->CopyFrom(entry);
  res->mutable_header()->mutable_src()->CopyFrom(entry);

  /**
   * initialize the ring
   */
  if (!router->successor(0)) {
    logger->info("first node joining, setting the node as successor");
    router->set_successor(0, src, endpoint);
    router->set_predecessor(0, src, endpoint);
  }

  return Status::OK;
}

RouterEntry Service::successor(const uuid_t &uuid) {
  ServerContext serverContext;
  SuccessorRequest req;
  SuccessorResponse res;

  req.mutable_header()->CopyFrom(make_header(context));
  req.set_id(uuid);

  const auto status = successor(&serverContext, &req, &res);

  if (!status.ok()) throw__grpc_exception("failed to query successor", status);

  return res.successor();
}

Status Service::successor(ServerContext *serverContext, const SuccessorRequest *req, SuccessorResponse *res) {
  (void)serverContext;
  logger->trace("[successor] from {}@{} successor of? {}",
                req->header().src().uuid(), req->header().src().endpoint(),
                req->id());

  //--- destination of the request
  uuid_t id{req->id()};
  uuid_t self{context.uuid()};

  const auto successor = router->successor();
  // only node on the ring
  if(!successor) {
    logger->trace("[successor] im the only node in the ring, returning myself {}", self);

    RouterEntry entry;
    entry.set_uuid(self);
    entry.set_endpoint(context.bind_addr);

    res->mutable_successor()->CopyFrom(entry);
    return Status::OK;
  }

  if(id == self || id.between(self, successor->uuid)) {
    logger->trace("[successor] the requested id {} lies between self {} and my successor {}, returning successor", id.string(), self.string(), successor->uuid);

    //--- router entry
    RouterEntry entry;
    entry.set_uuid(successor->uuid);
    entry.set_endpoint(successor->endpoint);

    res->mutable_successor()->CopyFrom(entry);
  } else {
    logger->trace("[successor] trying to spawn client to forward request.");
    const auto status = make_client().successor(req, res);
    if (!status.ok()) {
      logger->warn("[successor] failed to query successor from client!");
    } else {
      logger->trace("spawned client, got result");
      logger->trace("entry {}@{}", res->successor().uuid(), res->successor().endpoint());
    }
    return status;
  }

  return Status::OK;
}

Status Service::stabilize(ServerContext *serverContext, const StabilizeRequest *req, StabilizeResponse *res) {
  (void)serverContext;
  logger->trace("stabilize from {}@{}", req->header().src().uuid(),
                req->header().src().endpoint());

  const auto predecessor = router->predecessor();
  if (predecessor) {
    logger->debug("returning predecessor {}", predecessor);

    RouterEntry entry;
    entry.set_uuid(predecessor->uuid);
    entry.set_endpoint(predecessor->endpoint);

    res->mutable_predecessor()->CopyFrom(entry);
  }

  return Status::OK;
}

Status Service::notify(ServerContext *serverContext, const NotifyRequest *req, NotifyResponse *res) {
  (void)serverContext;
  (void)res;
  logger->trace("notify from {}@{}", req->header().src().uuid(),
                req->header().src().endpoint());

  const auto predecessor = router->predecessor();

  //--- validate
  if (!req->has_header() && !req->header().has_src()) {
    logger->warn("failed to validate header");
    return Status::CANCELLED;
  }

  const uuid_t self{context.uuid()};
  const uuid_t uuid{req->header().src().uuid()};
  const auto endpoint = req->header().src().endpoint();

  if (!predecessor
      || uuid.between(predecessor->uuid, self)) {
    router->set_predecessor(0, uuid, endpoint);
  } else {
    if (!predecessor)
      logger->info("predecessor is null");
    else {
      logger->info("n' = {}\npredecessor = {}\nself={}", uuid, predecessor, self);
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
  auto fix = context.uuid();
  if (!router->successor(0)) {
    fix += uuid_t{pow(2., (double) index - 1)};
    logger->trace("fix_fingers router successor is not null, increasing uuid");
  }

  logger->trace("fixing finger for {}.", fix);

  try {
    const auto entry = successor(fix);
    logger->trace("fixing finger for {}. received successor {}@{}", fix,
                  entry.uuid(), entry.endpoint());
    if( uuid_t{entry.uuid()} == context.uuid() ) {
      router->reset(fix);
      return;
    } 

    router->set_successor(index, uuid_t{entry.uuid()}, entry.endpoint());
  } catch (const chord::exception &e) {
    logger->warn("failed to fix fingers for {}", fix);
  }
}

} //namespace chord
