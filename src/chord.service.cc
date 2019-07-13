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

using grpc::ServerContext;
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

using namespace std;
using namespace chord::common;

namespace chord {

Service::Service(Context &context, Router *router, IClient* client)
    : context{context},
      router{router},
      client{client},
      logger{context.logging.factory().get_or_create(logger_name)} {}

Status Service::join(ServerContext *serverContext, const JoinRequest *req, JoinResponse *res) {
  (void)serverContext;
  const auto node = make_node(req->header().src());

  logger->trace("join request from {}", node);
  /**
   * find successor
   */
  const auto& src = node.uuid;

  const auto pred = router->predecessor();
  const auto succ = router->successor();

  /**
   * initialize the ring
   */
  if (!pred && (!succ || succ->uuid == context.uuid())) {
    logger->info("first node joining, setting the node as successor");
    router->set_successor(0, node);
  }
  /**
   * forward request to successor
   */
  else if(pred && !src.between(pred->uuid, context.uuid())) {
    const auto status = client->join(req, res);

    // node joined successfully? stabilize the ring
    if(status.ok()) {
      client->stabilize();
    }

    return status;
  }

  auto* res_succ = res->mutable_successor();
  auto* res_pred = res->mutable_predecessor();
  if(src.between(context.uuid(), succ->uuid)) {
    // successor
    const auto succ_or_self = succ.value_or(context.node());
    res_succ->set_uuid(succ_or_self.uuid);
    res_succ->set_endpoint(succ_or_self.endpoint);
    // predecessor
    const auto pred_or_self = pred.value_or(context.node());
    res_pred->set_uuid(pred_or_self.uuid);
    res_pred->set_endpoint(pred_or_self.endpoint);
    // update router
    router->set_successor(0, node);
  } else if(src.between(pred->uuid, context.uuid())) {
    // successor
    const auto successor = context.node();
    res_succ->set_uuid(successor.uuid);
    res_succ->set_endpoint(successor.endpoint);
    // predecessor
    const auto pred_or_self = pred.value_or(context.node());
    res_pred->set_uuid(pred_or_self.uuid);
    res_pred->set_endpoint(pred_or_self.endpoint);
  }

  res->mutable_header()->CopyFrom(make_header(context));

  return Status::OK;
}

RouterEntry Service::successor(const uuid_t &uuid) {
  ServerContext serverContext;
  SuccessorRequest req;
  SuccessorResponse res;

  req.mutable_header()->CopyFrom(make_header(context));
  req.set_id(uuid);

  const auto status = successor(&serverContext, &req, &res);

  if (!status.ok()) throw__grpc_exception(status);

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

  if(!successor) {
    logger->error("failed to query successor from this->router");
    return Status::CANCELLED;
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
    const auto status = client->successor(req, res);
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
  res->mutable_header()->CopyFrom(make_header(context));

  return Status::OK;
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
    const node new_successor = make_node(req->successor());
    const node leaving_node = make_node(req->header().src());

    router->replace_successor(leaving_node, new_successor);
    //event_leave(leaving_node, new_predecessor);
  }

  if(req->has_predecessor()) {
    const node new_predecessor = make_node(req->predecessor());
    const node leaving_node = make_node(req->header().src());

    router->replace_predecessor(leaving_node, new_predecessor);
    //event_leave(leaving_node, new_predecessor);
  } 

  return Status::OK;
}

Status Service::notify(ServerContext *serverContext, const NotifyRequest *req, NotifyResponse *res) {
  (void)serverContext;
  (void)res;

  //--- validate
  if (!req->has_header() && !req->header().has_src()) {
    logger->warn("failed to validate header");
    return Status::CANCELLED;
  }

  logger->trace("notify from {}@{}", req->header().src().uuid(),
                req->header().src().endpoint());

  const auto predecessor = router->predecessor();
  const uuid_t self{context.uuid()};
  const auto node = make_node(req->header().src());

  // note that we detect newly joined nodes by their 
  // notification, thus we must not set the predecessor
  // beforehand - even though we know the predecessor
  // within chord::Service::join we have to wait for
  // the joining node to notfiy its successor when it
  // is ready to handle requests
  if (!predecessor
      || node.uuid.between(predecessor->uuid, self)) {
    router->set_predecessor(0, node);
    event_joined(predecessor.value_or(context.node()), node);
  } else {
    if (!predecessor)
      logger->info("predecessor is null");
    else {
      logger->info("n' = {}\npredecessor = {}\nself={}", node.uuid, predecessor, self);
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
    fix += uuid_t{pow(2., static_cast<double>(index - 1))};
    logger->trace("fix_fingers router successor is not null, increasing uuid");
  }

  logger->trace("fixing finger for {}.", fix);

  try {
    const auto succ = make_node(successor(fix));
    logger->trace("fixing finger for {}. received successor {}", fix, succ);
    if( succ.uuid == context.uuid() ) {
      router->reset(fix);
      return;
    } 

    router->set_successor(index, succ);
  } catch (const chord::exception &e) {
    logger->warn("failed to fix fingers for {}", fix);
  }
}

} //namespace chord
