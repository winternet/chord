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

  if(!has_valid_header(req)) return Status::CANCELLED;

  const auto node = source_of(req);

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
    logger->info("first node joining, waiting for notify from joined node");
  }
  /**
   * forward request to successor
   */
  else if(pred && !src.between(context.uuid(), succ->uuid) && !src.between(pred->uuid, context.uuid())) {
    return client->join(req, res);
  }

  auto* res_succ = res->mutable_successor();
  auto* res_pred = res->mutable_predecessor();
  if(src.between(context.uuid(), succ->uuid)) {
    // successor
    const auto succ_or_self = succ.value_or(context.node());
    res_succ->set_uuid(succ_or_self.uuid);
    res_succ->set_endpoint(succ_or_self.endpoint);
    // predecessor
    const auto self = context.node();
    res_pred->set_uuid(self.uuid);
    res_pred->set_endpoint(self.endpoint);
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

  set_source(res, context);

  return Status::OK;
}

RouterEntry Service::successor(const uuid_t &uuid) {
  ServerContext serverContext;
  SuccessorRequest req = make_request<SuccessorRequest>(context);
  SuccessorResponse res;

  req.set_id(uuid);

  const auto status = successor(&serverContext, &req, &res);

  if (!status.ok()) throw__grpc_exception(status);

  return res.successor();
}

Status Service::successor(ServerContext *serverContext, const SuccessorRequest *req, SuccessorResponse *res) {
  (void)serverContext;

  if(!has_valid_header(req)) return Status::CANCELLED;

  const auto source = source_of(req);
  router->update(source);
  logger->trace("[successor] from {} successor of? {}", source, req->id());

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

  //TODO adapt test and enable?
  //if(!has_valid_header(req)) return Status::CANCELLED;

  const auto source = source_of(req);
  //TODO adapt test and uncomment?
  //router->update(source);
  logger->trace("stabilize from {}", source);

  const auto predecessor = router->predecessor();
  if (predecessor) {
    logger->debug("returning predecessor {}", predecessor);

    RouterEntry entry;
    entry.set_uuid(predecessor->uuid);
    entry.set_endpoint(predecessor->endpoint);

    res->mutable_predecessor()->CopyFrom(entry);
  }
  set_source(res, context);

  return Status::OK;
}

Status Service::leave(ServerContext *serverContext, const LeaveRequest *req, LeaveResponse *res) {
  (void)serverContext;
  (void)res;

  if(!has_valid_header(req)) return Status::CANCELLED;

  const auto leaving_node = source_of(req);

  logger->trace("[leave] received leaving node {}", leaving_node);

  router->remove(leaving_node);
  for(auto entry:req->entries()) {
    router->update(make_node(entry));
  }

  return Status::OK;
}

Status Service::notify(ServerContext *serverContext, const NotifyRequest *req, NotifyResponse *res) {
  (void)serverContext;
  (void)res;

  if(!has_valid_header(req)) return Status::CANCELLED;

  const auto source = source_of(req);

  logger->trace("[notify] from {}", source);

  const auto successor = router->successor();
  const auto predecessor = router->predecessor();
  const uuid_t self{context.uuid()};
  auto status = Status::OK;
  auto changed_successor = false;
  auto changed_predecessor = false;

  if(source == context.node()) {
    logger->warn("Error notifying: Unable to notify self.");
    return Status::CANCELLED;
  }

  // initializing ring
  if(!predecessor) {
    router->update(source);
    changed_successor = true;
    changed_predecessor = true;
  } else if(context.uuid().between(predecessor->uuid, source.uuid)) {
    changed_successor = true;
    router->update(source);
    //router->update_successor(*successor, source_node);
  } else if(context.uuid().between(source.uuid, successor->uuid)) {
    changed_predecessor = true;
    router->update(source);
    //router->set_predecessor(0, source_node);
  }

  if(req->has_old_node() && req->has_new_node()) {
    const auto old_node_ = make_node(req->old_node());
    const auto new_node_ = make_node(req->new_node());
    router->update(old_node_);
    router->update(new_node_);
  }
  if(changed_predecessor) {
    event_predecessor_update(predecessor.value_or(context.node()), source);
  }

  return status;
}

Status Service::check(ServerContext *serverContext, const CheckRequest *req, CheckResponse *res) {
  (void)serverContext;

  if(!has_valid_header(req)) return Status::CANCELLED;

  const auto source = source_of(req);
  logger->trace("[check] from {}", source);
  set_source(res, context);
  res->set_id(req->id());
  return Status::OK;
}

void Service::fix_fingers(size_t index) {
  const auto fix = router->calc_successor_uuid_for_index(index);

  logger->trace("fixing finger for {}.", fix);

  try {
    const auto succ = make_node(successor(fix));
    logger->trace("fixing finger for {}. received successor {}", fix, succ);
    if( succ.uuid == context.uuid() ) {
      router->remove(fix);
      return;
    } 

    router->update(succ);
  } catch (const chord::exception &e) {
    logger->warn("failed to fix fingers for {}", fix);
  }
}

} //namespace chord
