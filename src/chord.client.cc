#include "chord.client.h"

#include <memory>
#include <fstream>
#include <string>

#include <grpcpp/channel.h>
#include <grpcpp/create_channel.h>

#include "chord.grpc.pb.h"
#include "chord.pb.h"

#include "chord.log.h"
#include "chord.context.h"
#include "chord.common.h"
#include "chord.exception.h"
#include "chord.log.factory.h"
#include "chord.log.h"
#include "chord.node.h"
#include "chord.router.h"
#include "chord.uuid.h"

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;

using chord::common::Header;
using chord::common::RouterEntry;
using chord::common::make_node;
using chord::common::make_header;
using chord::common::set_source;

using chord::LookupResponse;
using chord::LookupRequest;
using chord::NotifyResponse;
using chord::NotifyRequest;
using chord::Chord;

using namespace std;

namespace chord {
Client::Client(const Context &context, Router *router)
    : context{context},
      router{router},
      make_stub{//--- default stub factory
                [](const endpoint& endpoint) {
                  return Chord::NewStub(grpc::CreateChannel(
                      endpoint, grpc::InsecureChannelCredentials()));
                }},
      logger{context.logging.factory().get_or_create(logger_name)} {}

Client::Client(const Context &context, Router *router, StubFactory make_stub)
    : context{context}, router{router}, make_stub{make_stub}, logger{context.logging.factory().get_or_create(logger_name)} {}

Status Client::inform_successor_about_leave() {
  // get successor
  const auto successor_node = router->successor();
  const auto predecessor_node = router->predecessor();

  if(successor_node == context.node()) {
    logger->info("[leave] no successor - shutting down");
    return Status(grpc::StatusCode::NOT_FOUND, "no successor found.");
  }

  ClientContext clientContext;
  LeaveRequest req;
  LeaveResponse res;

  // inform successor
  logger->trace("[leave] leaving chord ring, informing {}", successor_node);

  req.mutable_header()->CopyFrom(make_header(context));
  const auto predecessor = req.mutable_predecessor();
  predecessor->set_uuid(predecessor_node->uuid);
  predecessor->set_endpoint(predecessor_node->endpoint);
  return make_stub(successor_node->endpoint)->leave(&clientContext, req, &res);
}

Status Client::inform_predecessor_about_leave() {
  const auto successor_node = router->successor();
  const auto predecessor_node = router->predecessor();

  if(!predecessor_node) {
    logger->info("[leave] no predecessor - shutting down");
    return Status(grpc::StatusCode::NOT_FOUND, "no predecessor found.");
  }

  // inform predecessor
  ClientContext clientContext;
  LeaveRequest req;
  LeaveResponse res;

  logger->trace("[leave] leaving chord ring, informing {}", predecessor_node);

  req.mutable_header()->CopyFrom(make_header(context));
  const auto successor = req.mutable_successor();
  successor->set_uuid(successor_node->uuid);
  successor->set_endpoint(successor_node->endpoint);
  return make_stub(predecessor_node->endpoint)->leave(&clientContext, req, &res);
}

void Client::leave() {

  const auto status_succ = inform_successor_about_leave();
  if (!status_succ.ok()) {
    logger->warn("[leave] failed to inform successor about leave");
  }
  const auto status_pred = inform_predecessor_about_leave();
  if (!status_pred.ok()) {
    logger->warn("[leave] failed to inform predecessor about leave");
  }

}

signal<void(const node)>& Client::on_predecessor_fail() {
  return event_predecessor_fail;
}
signal<void(const node)>& Client::on_successor_fail() {
  return event_successor_fail;
}

Status Client::ping(const chord::node& node) {
  ClientContext clientContext;
  PingRequest req;
  PingResponse res;

  set_source(req, context);
  return make_stub(node.endpoint)->ping(&clientContext, req, &res);
}

Status Client::state(const chord::node& node, const bool predecessor, const bool finger) {
  ClientContext clientContext;
  StateRequest req;
  StateResponse res;

  set_source(req, context);

  req.set_predecessor(predecessor);
  req.set_finger(finger);

  const auto status = make_stub(node.endpoint)->state(&clientContext, req, &res);

  if(status.ok()) {
    handle_state_response(res);
  }

  return status;
}

void Client::handle_state_response(const StateResponse& res) {
  if(res.has_predecessor()) 
    router->update(make_node(res.predecessor()));
  if(res.finger_size() > 0)
    for(const auto f : res.finger())
      router->update(make_node(f));
}

Status Client::join(const endpoint& addr) {
  if(addr == context.bind_addr) {
    logger->error("[join] cannot join self");
    return Status::CANCELLED;
  }

  logger->debug("[join] joining {}", addr);

  ClientContext clientContext;
  LookupRequest req;
  LookupResponse res;

  set_source(req, context);
  //req.mutable_header()->CopyFrom(make_header(context));
  auto status = make_stub(addr)->lookup(&clientContext, req, &res);

  if(!status.ok()) {
    logger->info("[join] failed to join {}", addr);
    return status;
  }

  ClientContext stateClientContext;
  StateRequest stateReq;
  StateResponse stateRes;
  set_source(stateReq, context);
  stateReq.set_finger(true);
  stateReq.set_predecessor(true);
  status = make_stub(addr)->state(&stateClientContext, stateReq, &stateRes);

  if(!status.ok()) {
    logger->info("[join] failed to join {}", addr);
    return status;
  }

  handle_state_response(stateRes);

  logger->info("[join] successfully joined {}, pred {}, succ {}", addr, *router->predecessor(), *router->successor());

  return status;
}

Status Client::notify(const node& target, const node& old_node, const optional<node>& new_node) {
  // get successor
  //const auto n = router->successor();
  const auto successor = target.uuid;
  const auto endpoint = target.endpoint;

  ClientContext clientContext;
  NotifyRequest req;
  NotifyResponse res;

  logger->trace("[notify] calling notify on address {}@{}", successor, endpoint);

  // TODO rename proto
  req.mutable_header()->CopyFrom(make_header(context));
  const auto old_node_ = req.mutable_old_node();
  old_node_->set_uuid(old_node.uuid);
  old_node_->set_endpoint(old_node.endpoint);
  if(new_node) {
    const auto new_node_ = req.mutable_new_node();
    new_node_->set_uuid(new_node->uuid);
    new_node_->set_endpoint(new_node->endpoint);
  }
  return make_stub(endpoint)->notify(&clientContext, req, &res);
}

Status Client::notify() {

  // get successor
  const auto successor = router->successor();
  if(!successor) {
    logger->trace("[notify] no successor to notify.");
  }

  ClientContext clientContext;
  NotifyRequest req;
  NotifyResponse res;

  logger->trace("[notify] calling notify on address {}", *successor);

  req.mutable_header()->CopyFrom(make_header(context));
  return make_stub(successor->endpoint)->notify(&clientContext, req, &res);
}

Status Client::lookup(ClientContext *clientContext, const LookupRequest *req, LookupResponse *res) {

  logger->trace("[successor] trying to find successor of {}", req->id());
  LookupRequest copy(*req);
  copy.mutable_header()->CopyFrom(make_header(context));

  const auto predecessor = router->closest_preceding_node(uuid_t(req->id()));
  // this node is the closest preceding node -> successor is node's direct successor
  //if(predecessor && *predecessor == context.node()) {
  //  logger->trace("[successor] this node seems to be the closest preceding node");
  //  auto succ = res->mutable_successor();
  //  succ->set_uuid(context.uuid());
  //  succ->set_endpoint(context.bind_addr);
  //  return Status::OK;
  //}
  logger->trace("[successor] forwarding request to {}", predecessor);

  return predecessor ? make_stub(predecessor->endpoint)->lookup(clientContext, copy, res) : Status::CANCELLED;
}

/** called by chord.service **/
Status Client::lookup(const LookupRequest *req, LookupResponse *res) {
  ClientContext clientContext;
  return lookup(&clientContext, req, res);
}

RouterEntry Client::lookup(const uuid_t &uuid) {
  ClientContext clientContext;
  LookupRequest req;
  req.mutable_header()->CopyFrom(make_header(context));
  req.set_id(uuid);
  LookupResponse res;

  const auto status = lookup(&clientContext, &req, &res);

  if (!status.ok()) throw__grpc_exception(status);

  return res.successor();
}

}
