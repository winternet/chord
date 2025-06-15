#include "chord.service.h"

#include <grpcpp/impl/codegen/server_context.h>
#include <grpcpp/impl/codegen/status_code_enum.h>
#include <cmath>
#include <iostream>
#include <string>
#include <vector>

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

using namespace std;
using namespace chord::common;

namespace chord {

Service::Service(Context &context, Router *router, IClient* client)
    : context{context},
      router{router},
      client{client},
      logger{context.logging.factory().get_or_create(logger_name)} {}

Status Service::join([[maybe_unused]] ServerContext *serverContext, const JoinRequest *req, JoinResponse *res) {

  if(!has_valid_header(req)) return Status::CANCELLED;
  const auto peer = serverContext->peer();

  const auto node = source_of(serverContext, req);

  logger->trace("join request from {}", node);

  if(client->ping(node).ok())
    logger->trace("[+] verified reverse connection to {}", node);
  else {
    logger->trace("[-] node is not reachable ({}) - continue.", node);
    //return Status::CANCELLED;
  }
  /**
   * find successor
   */
  const auto& src = node.uuid;

  const auto pred = router->predecessor();
  const auto succ = router->successor_or_self();

  /**
   * initialize the ring
   */
  if (!pred && (succ.uuid == context.uuid())) {
    logger->info("first node joining, waiting for notify from joined node");
  }
  /**
   * forward request to successor
   */
  else if(pred && !uuid::between(context.uuid(), src, succ.uuid) 
      && !uuid::between(pred->uuid, src, context.uuid())) {
    return client->join(req, res);
  }

  auto* res_succ = res->mutable_successor();
  auto* res_pred = res->mutable_predecessor();
  if(uuid::between(context.uuid(), src, succ.uuid)) {
    // successor
    //const auto succ_or_self = succ.value_or(context.node());
    //res_succ->set_uuid(succ_or_self.uuid);
    //res_succ->set_endpoint(succ_or_self.endpoint);
    res_succ->set_uuid(succ.uuid);
    res_succ->set_endpoint(succ.endpoint);
    // predecessor
    const auto self = context.node();
    res_pred->set_uuid(self.uuid);
    res_pred->set_endpoint(self.endpoint);
  } else if(uuid::between(pred->uuid, src, context.uuid())) {
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
  auto req = make_request<SuccessorRequest>(context);
  SuccessorResponse res;

  req.set_id(uuid);

  const auto status = successor(&serverContext, &req, &res);

  if (!status.ok()) throw__grpc_exception(status);

  return res.successor();
}

Status Service::successor([[maybe_unused]] ServerContext *serverContext, const SuccessorRequest *req, SuccessorResponse *res) {

  if(!has_valid_header(req)) return Status::CANCELLED;

  const auto source = source_of(req);
  router->update(source);
  logger->trace("[successor] from {} successor of? {}", source, req->id());

  //--- destination of the request
  uuid_t id{req->id()};
  uuid_t self{context.uuid()};

  auto successor = router->successor_or_self();

  while(!client->ping(successor).ok()) {
    router->remove(successor);
    successor = router->successor_or_self();
  }

  if(id == self || uuid::between(self, id, successor.uuid)) {
    logger->trace("[successor] the requested id {} lies between self {} and my successor {}, returning successor", id.string(), self.string(), successor.uuid);

    //--- router entry
    RouterEntry entry = make_entry(successor);

    res->mutable_successor()->CopyFrom(entry);
    
    return Status::OK;
  } 

  return client->successor(req, res);
}

Status Service::stabilize([[maybe_unused]] ServerContext *serverContext, const StabilizeRequest *req, StabilizeResponse *res) {

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

Status Service::leave([[maybe_unused]] ServerContext *serverContext, const LeaveRequest *req, [[maybe_unused]] LeaveResponse *res) {

  if(!has_valid_header(req)) return Status::CANCELLED;

  const auto leaving_node = source_of(req);

  logger->trace("[leave] received leaving node {}", leaving_node);

  // no need to signal fails of predecessor
  router->remove(leaving_node, false);
  for(auto entry:req->entries()) {
    router->update(make_node(serverContext, entry));
  }

  return Status::OK;
}

Status Service::notify([[maybe_unused]] ServerContext *serverContext, const NotifyRequest *req, [[maybe_unused]] NotifyResponse *res) {

  if(!has_valid_header(req)) return Status::CANCELLED;

  const auto source = source_of(req);

  logger->trace("[notify] from {}", source);

  const auto successor = router->successor();
  const auto predecessor = router->predecessor();
  const uuid_t self{context.uuid()};
  auto status = Status::OK;
  auto changed_predecessor = false;

  if(source == context.node()) {
    logger->warn("[notify] received notify from self - aborting.");
    return Status::CANCELLED;
  }

  // initializing ring
  if(!predecessor) {
    changed_predecessor = true;
  //TODO this branch is useless but prevents invocation of following cond..
  //} else if(uuid::between(predecessor->uuid, context.uuid(), source.uuid)) {
  } else if(predecessor && predecessor != source 
      && uuid::between(source.uuid, context.uuid(), successor->uuid)) {
    changed_predecessor = true;
  }
  // update router
  router->update(source);

  if(req->has_old_node() && req->has_new_node()) {
    const auto old_node_ = make_node(serverContext, req->old_node());
    const auto new_node_ = make_node(serverContext, req->new_node());
    router->update(old_node_);
    router->update(new_node_);
  }
  if(changed_predecessor) {
    event_predecessor_update(predecessor.value_or(context.node()), source);
  }

  return status;
}

Status Service::ping([[maybe_unused]] ServerContext *serverContext, const PingRequest *req, PingResponse *res) {

  if(!has_valid_header(req)) return Status::CANCELLED;

  const auto source = source_of(req);
  logger->trace("[ping] from {}", source);
  set_source(res, context);
  return Status::OK;
}

void Service::fix_fingers(size_t index) {
  const auto fix = router->calc_successor_uuid_for_index(index);

  logger->trace("fixing finger for {}.", fix);

  try {
    const auto succ = make_node(successor(fix));
    logger->trace("fixing finger for {}. received successor {}", fix, succ);
    if( succ == context.node() ) {
      router->remove(fix);
      return;
    } 

    router->update(succ);
  } catch (const chord::exception &e) {
    logger->warn("failed to fix fingers for {}", fix);
  }
}

} //namespace chord
