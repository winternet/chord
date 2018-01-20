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

#define log(level) LOG(level) << "[service] "

#define SERVICE_LOG(level, method) LOG(level) << "[service][" << #method << "] "

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
    : context{context}, router{router} {
  make_client = [&]() {
    return chord::Client(context, router);
  };
}

Service::Service(Context &context, Router *router, ClientFactory make_client)
    : context{context}, router{router}, make_client{make_client} {}

Status Service::join(ServerContext *serverContext, const JoinRequest *req, JoinResponse *res) {
  (void)serverContext;
  auto id = req->header().src().uuid();
  auto endpoint = req->header().src().endpoint();

  SERVICE_LOG(trace, join) << "from " << id << "@" << endpoint;
  /**
   * find successor
   */
  auto src = uuid_t{id};
  auto entry = successor(src);

  res->mutable_successor()->CopyFrom(entry);

  /**
   * initialize the ring
   */
  if (router->successors[0]==nullptr) {
    SERVICE_LOG(info, join) << "first node joining, setting the node as successor";
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

  if (!status.ok()) throw chord::exception("failed to query successor", status);

  return res.successor();
}

Status Service::successor(ServerContext *serverContext, const SuccessorRequest *req, SuccessorResponse *res) {
  (void)serverContext;
  SERVICE_LOG(trace, successor) << "from " << req->header().src().uuid()
                                << "@" << req->header().src().endpoint()
                                << " successor of? " << req->id();

  //--- destination of the request
  uuid_t id(req->id());
  uuid_t self(context.uuid());
  uuid_t successor(*router->successor());

  SERVICE_LOG(trace, successor) << "id: " << id << ", self: " << self << ", successor " << successor;

  if (id > self and (id <= successor or (successor < self and id > successor))) {
    SERVICE_LOG(trace, successor) << "of " << id << " is " << successor;
    //--- router entry
    RouterEntry entry;
    entry.set_uuid(successor);
    entry.set_endpoint(router->get(successor));

    res->mutable_successor()->CopyFrom(entry);
  } else {
    SERVICE_LOG(trace, successor) << "id not within self(" << context.uuid() << ") <--> successor("
                                  << *router->successor() << ") trying to forward.";
    uuid_t next = router->closest_preceding_node(id);
    SERVICE_LOG(trace, successor) << "CLOSEST PRECEDING NODE: " << next;
    if (next==self) {
      RouterEntry entry;
      entry.set_uuid(self);
      entry.set_endpoint(router->get(self));

      res->mutable_successor()->CopyFrom(entry);
    } else {
      SERVICE_LOG(trace, successor) << "trying to spawn client to forward request";
      Status status = make_client().successor(req, res);
      if (!status.ok()) {
        SERVICE_LOG(warning, successor) << "failed to query successor from client!";
      } else {
        SERVICE_LOG(trace, successor) << "spawned client, got result";
        SERVICE_LOG(trace, successor) << "entry: " << res->successor().uuid() << "@" << res->successor().endpoint();
      }
    }
  }

  return Status::OK;
}

Status Service::stabilize(ServerContext *serverContext, const StabilizeRequest *req, StabilizeResponse *res) {
  (void)serverContext;
  SERVICE_LOG(trace, stabilize) << "from " << req->header().src().uuid()
                                << "@" << req->header().src().endpoint();

  auto predecessor = router->predecessor();
  if (predecessor!=nullptr) {
    SERVICE_LOG(debug, stabilize) << "returning predecessor " << *predecessor;
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
  SERVICE_LOG(trace, notify) << "from " << req->header().src().uuid()
                             << "@" << req->header().src().endpoint();

  auto predecessor = router->predecessor();

  //--- validate
  if (!req->has_header() && !req->header().has_src()) {
    log(warning) << "failed to validate header";
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
      log(info) << "predecessor is null";
    else {
      log(info) << "n' = " << uuid << "\npredecessor = " << *predecessor << "\nself = " << self;
    }
  }

  return Status::OK;
}

Status Service::check(ServerContext *serverContext, const CheckRequest *req, CheckResponse *res) {
  (void)serverContext;
  SERVICE_LOG(trace, check) << "from " << req->header().src().uuid()
                            << "@" << req->header().src().endpoint();
  res->mutable_header()->CopyFrom(make_header(context));
  res->set_id(req->id());
  return Status::OK;
}

void Service::fix_fingers(size_t index) {
  uuid_t fix = context.uuid();
  if (router->successors[0]!=nullptr) {
    fix += uuid_t(pow(2., (double) index - 1));
    SERVICE_LOG(trace, fix_fingers) << " router successor is not null, increasing uuid ";
  }

  SERVICE_LOG(trace, fix_fingers) << "fixing finger for " << fix << ".";

  try {
    auto entry = successor(fix);
    SERVICE_LOG(trace, fix_fingers) << "fixing finger for " << fix << ". received successor " << entry.uuid()
                                    << "@" << entry.endpoint();
    //if( uuid_t(router_entry.uuid()) == context.uuid() ) return;

    router->set_successor(index, uuid_t{entry.uuid()}, entry.endpoint());
  } catch (const chord::exception &e) {
    SERVICE_LOG(warning, fix_fingers) << "failed to fix fingers for " << fix;
  }
}

} //namespace chord
