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

Status Client::inform_successour_about_leave() {
  // get successor
  const auto successor_node = router->successor();
  const auto predecessor_node = router->predecessor();

  if(successor_node == context.node()) {
    logger->info("[leaving] no successor - shutting down");
    return Status(grpc::StatusCode::NOT_FOUND, "no successor found.");
  }

  ClientContext clientContext;
  LeaveRequest req;
  LeaveResponse res;

  // inform successor
  logger->trace("leaving chord ring, informing {}", successor_node);

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
    logger->info("[leaving] no predecessor - shutting down");
    return Status(grpc::StatusCode::NOT_FOUND, "no predecessor found.");
  }

  // inform predecessor
  ClientContext clientContext;
  LeaveRequest req;
  LeaveResponse res;

  logger->trace("leaving chord ring, informing {}", predecessor_node);

  req.mutable_header()->CopyFrom(make_header(context));
  const auto successor = req.mutable_successor();
  successor->set_uuid(successor_node->uuid);
  successor->set_endpoint(successor_node->endpoint);
  return make_stub(predecessor_node->endpoint)->leave(&clientContext, req, &res);
}

void Client::leave() {

  const auto status_succ = inform_successour_about_leave();
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

bool Client::join(const endpoint& addr) {
  logger->debug("joining {}", addr);

  ClientContext clientContext;
  JoinRequest req;

  req.mutable_header()->CopyFrom(make_header(context));

  JoinResponse res;
  const auto status = make_stub(addr)->join(&clientContext, req, &res);

  if (!status.ok() || !res.has_successor()) {
    logger->info("Failed to join {}", addr);
    return false;
  }

  const auto succ = make_node(res.successor());
  const auto pred = make_node(res.predecessor());

  logger->info("Successfully joined {}", addr);
  router->set_predecessor(0, pred);
  router->set_successor(0, succ);
  return true;
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
  StabilizeRequest req;
  StabilizeResponse res;

  req.mutable_header()->CopyFrom(make_header(context));

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
    router->reset(*successor);
    event_successor_fail(*successor);
    return;
  }

  if (res.has_predecessor()) {
    const auto pred = make_node(res.predecessor());
    logger->trace("received stabilize response with predecessor {}", pred);

    const uuid_t self(context.uuid());
    const uuid_t succ(router->successor()->uuid);
    if(pred.uuid.between(self, succ)) {
      router->set_successor(0, pred);
    }
  } else {
    logger->trace("received empty routing entry");
  }

  notify();
}

void Client::notify() {

  // get successor
  const auto n = router->successor();
  const auto successor = n->uuid;
  const auto endpoint = n->endpoint;

  ClientContext clientContext;
  NotifyRequest req;
  NotifyResponse res;

  logger->trace("calling notify on address {}@{}", successor, endpoint);

  req.mutable_header()->CopyFrom(make_header(context));
  make_stub(endpoint)->notify(&clientContext, req, &res);

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
  //const auto endpoint = router->get(predecessor);
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
  SuccessorRequest req;
  req.mutable_header()->CopyFrom(make_header(context));
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
  CheckRequest req;
  CheckResponse res;

  req.mutable_header()->CopyFrom(make_header(context));

  const auto endpoint = predecessor->endpoint;//router->get(predecessor);

  logger->trace("checking predecessor {}", *predecessor);
  const auto status = make_stub(endpoint)->check(&clientContext, req, &res);

  if (!status.ok()) {
    logger->warn("[check] predecessor failed.");
    router->reset(*predecessor);
    event_predecessor_fail(*predecessor);
  } else if(!res.has_header()) {
    logger->error("[check] returned without header, should remove endpoint {}@{}?", *predecessor, endpoint);
  }
}
}
