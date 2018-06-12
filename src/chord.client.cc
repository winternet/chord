#include <memory>
#include <fstream>

#include <grpc++/channel.h>
#include <grpc++/create_channel.h>

#include "chord.log.h"
#include "chord.uuid.h"
#include "chord.client.h"
#include "chord.router.h"
#include "chord.exception.h"

#include "chord.common.h"
#include "chord.crypto.h"


using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using grpc::ClientWriter;
using grpc::ClientReader;

using chord::common::Header;
using chord::common::RouterEntry;

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
using namespace chord::common;

namespace chord {
Client::Client(const Context &context, Router *router)
    : context{context},
      router{router},
      make_stub{//--- default stub factory
                [](const endpoint_t &endpoint) {
                  return Chord::NewStub(grpc::CreateChannel(
                      endpoint, grpc::InsecureChannelCredentials()));
                }},
      logger{log::get_or_create(logger_name)} {}

Client::Client(const Context &context, Router *router, StubFactory make_stub)
    : context{context}, router{router}, make_stub{make_stub}, logger{log::get_or_create(logger_name)} {}

void Client::join(const endpoint_t &addr) {
  logger->debug("joining {}", addr);

  ClientContext clientContext;
  JoinRequest req;

  req.mutable_header()->CopyFrom(make_header(context));

  JoinResponse res;
  Status status = make_stub(addr)->join(&clientContext, req, &res);

  if (!status.ok() || !res.has_successor()) {
    throw__exception("Failed to join " + addr);
  }

  auto entry = res.successor();
  auto id = entry.uuid();
  auto endpoint = entry.endpoint();

  logger->trace("successful, received successor {}@{}", id, endpoint);
  router->set_successor(0, uuid_t{id}, endpoint);
}

void Client::stabilize() {
  ClientContext clientContext;
  StabilizeRequest req;
  StabilizeResponse res;

  req.mutable_header()->CopyFrom(make_header(context));

  auto successor = router->successor();

  //--- return if join failed or uuid == successor (create)
  if (successor==nullptr || *successor == context.uuid()) {
    logger->trace("[stabilize] no successor found");
    return;
  }

  endpoint_t endpoint = router->get(successor);

  logger->trace("[stabilize] calling stabilize on successor {}", endpoint);
  Status status = make_stub(endpoint)->stabilize(&clientContext, req, &res);

  if (!status.ok()) {
    logger->warn("[stabilize] failed - removing endpoint {}@{}?", *successor, endpoint);
    router->reset(*successor);
    return;
  }

  if (res.has_predecessor()) {
    logger->trace("received stabilize response with predecessor {}@{}", res.predecessor().uuid(), res.predecessor().endpoint());
    const RouterEntry &entry = res.predecessor();

    uuid_t self(context.uuid());
    uuid_t pred(entry.uuid());
    uuid_t succ(*router->successor());
    if(pred.between(self, succ)) {
      router->set_successor(0, pred, entry.endpoint());
    }
  } else {
    logger->trace("received empty routing entry");
  }

  notify();
}

void Client::notify() {

  // get successor
  auto successor = router->successor();
  endpoint_t endpoint = router->get(successor);

  ClientContext clientContext;
  NotifyRequest req;
  NotifyResponse res;

  logger->trace("calling notify on address {}@{}", *successor, endpoint);

  req.mutable_header()->CopyFrom(make_header(context));
  make_stub(endpoint)->notify(&clientContext, req, &res);

}

Status Client::successor(ClientContext *clientContext, const SuccessorRequest *req, SuccessorResponse *res) {

  logger->trace("trying to find successor of {}", req->id());
  SuccessorRequest copy(*req);
  copy.mutable_header()->CopyFrom(make_header(context));

  uuid_t predecessor = router->closest_preceding_node(uuid_t(req->id()));
  endpoint_t endpoint = router->get(predecessor);
  logger->trace("forwarding request to {}", endpoint);

  return make_stub(endpoint)->successor(clientContext, copy, res);
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

  auto status = successor(&clientContext, &req, &res);

  if (!status.ok()) throw__grpc_exception("failed to query succesor", status);

  return res.successor();
}

void Client::check() {
  auto predecessor = router->predecessor();
  auto successor = router->successor();

  if (predecessor == nullptr) {
    logger->trace("[check] no predecessor, skip.");
    return;
  }
  if (successor == nullptr) {
    logger->trace("no successor, skip.");
    return;
  }

  ClientContext clientContext;
  CheckRequest req;
  CheckResponse res;

  req.mutable_header()->CopyFrom(make_header(context));

  auto endpoint = router->get(predecessor);

  logger->trace("checking predecessor {}@{}", *predecessor, endpoint);
  const grpc::Status status = make_stub(endpoint)->check(&clientContext, req, &res);

  if (!status.ok()) {
    logger->warn("[check] predecessor failed.");
    router->reset(*predecessor);
  } else if(!res.has_header()) {
    logger->error("[check] returned without header, should remove endpoint {}@{}?", *predecessor, endpoint);
  }
}
}
