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
using chord::common::make_entry;
using chord::common::make_header;
using chord::common::set_source;
using chord::common::make_request;

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

//! TODO merge with inform predecessor about leave
Status Client::inform_successor_about_leave() {
  // get successor
  const auto successor_node = router->successor();
  const auto predecessor_node = router->predecessor();

  if(successor_node == context.node()) {
    logger->info("[leave] no successor - shutting down");
    return Status(grpc::StatusCode::NOT_FOUND, "no successor found.");
  }

  // inform successor
  ClientContext clientContext;
  LeaveRequest req;
  LeaveResponse res;

  logger->trace("[leave] informing successor {}", successor_node);

  set_source(req, context);

  //req.mutable_header()->CopyFrom(make_header(context));
  auto entries = req.mutable_entries();
  for(const auto node : router->get()) {
    entries->Add(make_entry(node));
  }
  return make_stub(successor_node->endpoint)->leave(&clientContext, req, &res);
}

//! TODO merge with inform successor about leave
Status Client::inform_predecessor_about_leave() {
  const auto successor_node = router->successor();
  const auto predecessor_node = router->predecessor();

  if(!predecessor_node) {
    logger->info("[leave] no predecessor - shutting down");
    return Status(grpc::StatusCode::NOT_FOUND, "no predecessor found.");
  }

  // inform predecessor
  ClientContext clientContext;
  LeaveRequest req = make_request<LeaveRequest>(context);
  LeaveResponse res;

  logger->trace("[leave] informing predecessor {}", predecessor_node);

  auto entries = req.mutable_entries();
  for(const auto node : router->get()) {
    entries->Add(make_entry(node));
  }
  return make_stub(predecessor_node->endpoint)->leave(&clientContext, req, &res);
}

void Client::leave() {

  const auto status_succ = inform_successor_about_leave();
  if (!status_succ.ok()) {
    logger->warn("failed to inform successor about leave");
  }
  const auto status_pred = inform_predecessor_about_leave();
  if (!status_pred.ok()) {
    logger->warn("failed to inform predecessor about leave");
  }

}

signal<void(const node)>& Client::on_predecessor_fail() {
  return event_predecessor_fail;
}
signal<void(const node)>& Client::on_successor_fail() {
  return event_successor_fail;
}

Status Client::join(const endpoint& addr) {
  logger->debug("joining {}", addr);

  ClientContext clientContext;
  JoinRequest req = make_request<JoinRequest>(context);

  JoinResponse res;
  const auto status = make_stub(addr)->join(&clientContext, req, &res);

  if (!status.ok() || !res.has_successor()) {
    logger->info("Failed to join {}", addr);
    return status;
  }

  const auto succ = make_node(res.successor());
  const auto pred = make_node(res.predecessor());

  logger->info("Successfully joined {}, pred {}, succ {}", addr, pred, succ);
  router->update(succ);
  router->update(pred);
  //router->set_predecessor(0, pred);
  //router->set_successor(0, succ);

  return status;
}

Status Client::join(const JoinRequest *req, JoinResponse *res) {
  ClientContext clientContext;
  return join(&clientContext, req, res);
}

Status Client::join(ClientContext *clientContext, const JoinRequest *req, JoinResponse *res) {

  logger->trace("forwarding join of {}", req->header().src().uuid());
  JoinRequest copy(*req);
  //copy.mutable_header()->CopyFrom(make_header(context));

  const auto predecessor = router->closest_preceding_node(uuid_t(req->header().src().uuid()));
  logger->trace("forwarding request to {}", predecessor);

  return predecessor ? make_stub(predecessor->endpoint)->join(clientContext, copy, res) : Status::CANCELLED;
}


void Client::stabilize() {
  ClientContext clientContext;
  StabilizeRequest req = make_request<StabilizeRequest>(context);
  StabilizeResponse res;

  const auto successor = router->successor();

  //--- return if join failed or uuid == successor (create)
  if (!successor || successor->uuid == context.uuid()) {
    logger->trace("[stabilize] no successor found");
    return;
  }

  const auto endpoint = successor->endpoint;

  logger->trace("[stabilize] calling stabilize on successor {}", endpoint);
  const auto status = make_stub(endpoint)->stabilize(&clientContext, req, &res);

  if (!status.ok()) {
    logger->warn("[stabilize] failed - removing endpoint {}?", endpoint);
    router->remove(*successor);
    event_successor_fail(*successor);
    return;
  }

  if (res.has_predecessor()) {
    const auto pred = make_node(res.predecessor());
    logger->trace("received stabilize response with predecessor {}", pred);

    const uuid_t self(context.uuid());
    const uuid_t succ(router->successor()->uuid);
    if(pred.uuid.between(self, succ)) {
      router->update(pred);
      //router->set_successor(0, pred);
    }
  } else {
    logger->trace("received empty routing entry");
  }

  notify();
}

Status Client::notify(const node& target, const node& old_node, const node& new_node) {
  // get successor
  //const auto n = router->successor();
  const auto successor = target.uuid;
  const auto endpoint = target.endpoint;

  ClientContext clientContext;
  NotifyRequest req = make_request<NotifyRequest>(context);
  NotifyResponse res;

  logger->trace("calling notify on address {}@{}", successor, endpoint);

  // TODO rename proto
  const auto old_node_ = req.mutable_old_node();
  old_node_->set_uuid(old_node.uuid);
  old_node_->set_endpoint(old_node.endpoint);
  const auto new_node_ = req.mutable_new_node();
  new_node_->set_uuid(new_node.uuid);
  new_node_->set_endpoint(new_node.endpoint);
  return make_stub(endpoint)->notify(&clientContext, req, &res);
}

Status Client::notify() {

  // get successor
  const auto n = router->successor();
  const auto successor = n->uuid;
  const auto endpoint = n->endpoint;

  ClientContext clientContext;
  NotifyRequest req = make_request<NotifyRequest>(context);
  NotifyResponse res;

  logger->trace("calling notify on address {}@{}", successor, endpoint);

  return make_stub(endpoint)->notify(&clientContext, req, &res);
}

Status Client::successor(ClientContext *clientContext, const SuccessorRequest *req, SuccessorResponse *res) {

  logger->trace("trying to find successor of {}", req->id());
  SuccessorRequest copy(*req);
  copy.mutable_header()->CopyFrom(make_header(context));

  const auto predecessor = router->closest_preceding_node(uuid_t(req->id()));
  // this node is the closest preceding node -> successor is node's direct successor
  if(predecessor && *predecessor == context.node()) {
    logger->trace("this node seems to be the closest preceding node");
    auto succ = res->mutable_successor();
    succ->set_uuid(context.uuid());
    succ->set_endpoint(context.bind_addr);
    return Status::OK;
  }
  logger->trace("forwarding request to {}", predecessor);

  return predecessor ? make_stub(predecessor->endpoint)->successor(clientContext, copy, res) : Status::CANCELLED;
}

/** called by chord.service **/
Status Client::successor(const SuccessorRequest *req, SuccessorResponse *res) {
  ClientContext clientContext;
  return successor(&clientContext, req, res);
}

RouterEntry Client::successor(const uuid_t &uuid) {
  ClientContext clientContext;
  SuccessorRequest req = make_request<SuccessorRequest>(context);
  req.set_id(uuid);
  SuccessorResponse res;

  const auto status = successor(&clientContext, &req, &res);

  if (!status.ok()) throw__grpc_exception(status);

  return res.successor();
}

void Client::check() {
  const auto predecessor = router->predecessor();
  const auto successor = router->successor();

  if (!predecessor) {
    logger->trace("[check] no predecessor, skip.");
    return;
  }
  if (!successor) {
    logger->trace("no successor, skip.");
    return;
  }

  ClientContext clientContext;
  CheckRequest req = make_request<CheckRequest>(context);
  CheckResponse res;

  const auto endpoint = predecessor->endpoint;//router->get(predecessor);

  logger->trace("checking predecessor {}", *predecessor);
  const auto status = make_stub(endpoint)->check(&clientContext, req, &res);

  if (!status.ok()) {
    logger->warn("[check] predecessor failed.");
    router->remove(*predecessor);
    event_predecessor_fail(*predecessor);
  } else if(!res.has_header()) {
    logger->error("[check] returned without header, should remove endpoint {}@{}?", *predecessor, endpoint);
  }
}
}
