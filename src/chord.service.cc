#include "chord.service.h"

#include <grpcpp/impl/codegen/server_context.h>
#include <grpcpp/impl/codegen/status_code_enum.h>
#include <cmath>
#include <iostream>
#include <string>
#include <vector>

#include "chord.optional.h"
#include "chord.pb.h"
#include "chord.common.h"
#include "chord.context.h"
#include "chord.exception.h"
#include "chord.i.client.h"
#include "chord.log.factory.h"
#include "chord.log.h"
#include "chord.node.h"
#include "chord.router.h"
#include "chord.utils.h"

using grpc::ServerContext;
using grpc::Status;

using chord::common::Header;
using chord::LookupResponse;
using chord::LookupRequest;
using chord::NotifyResponse;
using chord::NotifyRequest;

using namespace std;
using namespace chord::common;

namespace chord {

Service::Service(Context &context, Router *router, IClient* client)
    : context{context},
      router{router},
      client{client},
      logger{context.logging.factory().get_or_create(logger_name)} {}

Status Service::ping(ServerContext *serverContext, const PingRequest *req, PingResponse *res) {
  (void)serverContext;
  (void)res;

  logger->trace("[ping] from {}", source_of(req));

  router->update(source_of(req));

  return Status::OK;
}

RouterEntry Service::lookup(const uuid_t &uuid) {
  ServerContext serverContext;
  LookupRequest req;
  LookupResponse res;

  set_source(req, context);

  req.set_id(uuid);

  const auto status = lookup(&serverContext, &req, &res);

  if (!status.ok()) throw__grpc_exception(status);

  return res.successor();
}

Status Service::state(ServerContext *serverContext, const StateRequest *req, StateResponse *res) {
  (void)serverContext;
  logger->trace("[state] from {}", source_of(req));

  if(req->predecessor())
    res->mutable_predecessor()->CopyFrom(make_entry(*router->predecessor()));
  if(req->finger()) {
    auto finger_field = res->mutable_finger();
    for(const auto f:router->finger()) {
      finger_field->Add(make_entry(f));
    }
  }

  return Status::OK;
}

Status Service::lookup(ServerContext *serverContext, const LookupRequest *req, LookupResponse *res) {
  (void)serverContext;
  logger->trace("[successor] from {}@{} successor of? {}",
                req->header().src().uuid(), req->header().src().endpoint(),
                req->id());

  //--- destination of the request
  uuid_t id{req->id()};
  uuid_t self{context.uuid()};

  logger->debug("[successor][dump] {}", *router);

  //TODO: use helper
  router->update(source_of(req));
  //set_source(res, context);

  const auto successor = router->successor();

  if(!successor) return Status::CANCELLED;

  if(self == id || id == successor->uuid || uuid::between(self, id, successor->uuid)) {
    logger->trace("[successor] the requested id {} lies between self {} and my successor {}, returning successor", id.string(), self.string(), successor->uuid);

    const auto status = client->ping(*successor);
    if(!status.ok()) {
      logger->error("[successor] pinging successor failed. {}", utils::to_string(status));
      return Status::CANCELLED;
      //TODO handle somehow
    }

    //--- router entry
    res->mutable_successor()->CopyFrom(make_entry(*successor));
    return Status::OK;
  }

  logger->trace("[successor] trying to spawn client to forward request.");
  const auto status = client->lookup(req, res);
  if (!status.ok()) {
    logger->warn("[successor] failed to query successor from client!");
  } else {
    logger->trace("spawned client, got result");
    logger->trace("entry {}@{}", res->successor().uuid(), res->successor().endpoint());
  }
  return status;
}

Status Service::leave(ServerContext *serverContext, const LeaveRequest *req, LeaveResponse *res) {
  (void)serverContext;
  (void)res;

  //--- validate
  if (!req->has_header() && !req->header().has_src()) {
    logger->warn("failed to validate header");
    return Status::CANCELLED;
  }

  logger->trace("received leaving node {}", make_node(req->header().src()));

  if(req->has_predecessor() && req->has_successor()) {
    logger->error("leave request contains predecessor AND successor.");
    return Status(grpc::StatusCode::INVALID_ARGUMENT, "leave request contains predecessor AND successor.");
  }
  // we trust the sender
  if(req->has_successor()) {
    router->remove(make_node(req->header().src()));
    router->update(make_node(req->successor()));
    //event_leaving(leaving_node, new_successor);
  }

  if(req->has_predecessor()) {
    router->remove(make_node(req->header().src()));
    router->update(make_node(req->predecessor()));
    //event_leave(leaving_node, new_predecessor);
  } 

  return Status::OK;
}

Status Service::notify(ServerContext *serverContext, const NotifyRequest *req, NotifyResponse *res) {
  (void)serverContext;
  (void)res;

  //--- validate
  if (!req->has_header() || !req->header().has_src()) {
    logger->warn("failed to validate header");
    return Status::CANCELLED;
  }

  logger->trace("notify from {}@{}", req->header().src().uuid(),
                req->header().src().endpoint());

  const auto successor = router->successor();
  const auto predecessor = router->predecessor();
  const uuid_t self{context.uuid()};
  const auto source_node = make_node(req->header().src());

  auto status = Status::OK;
  auto changed_successor = false;
  auto changed_predecessor = false;

  logger->trace("[notify] from {}", source_node);

  if(source_node == context.node()) {
    logger->warn("[notify] error notifying: Unable to notify self.");
    return Status::CANCELLED;
  }

  // initializing ring
  if(!router->has_predecessor()) {
    router->update(source_node);
    //check whether it makes sense to set both...
    changed_successor = true;
    changed_predecessor = true;

  } else if(context.uuid().between(source_node.uuid, successor->uuid)) {
    changed_predecessor = true;
    router->update(source_node);
  }

  if(changed_predecessor) {
    //TODO uncomment after tests
    //event_joined(predecessor.value_or(context.node()), source_node);
  }

  return status;
}

void Service::fix_fingers(size_t index) {
  const auto uuid = router->get(index);

  logger->trace("fixing finger for uuid {}.", uuid);

  try {
    const auto succ = make_node(lookup(uuid));
    logger->trace("fixing finger for {} - received successor {}", uuid, succ);
    if( succ.uuid == context.uuid() ) {
      router->remove(context.node());
      return;
    } 

    router->update(succ);
  } catch (const chord::exception &e) {
    logger->warn("failed to fix fingers for {}", uuid);
  }
  logger->warn("[dump] {}", *router);
}

} //namespace chord
