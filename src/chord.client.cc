#include <memory>
#include <fstream>

#include <grpc++/channel.h>
#include <grpc++/create_channel.h>

#include "chord.log.h"
#include "chord.uuid.h"
#include "chord.client.h"
#include "chord.router.h"
#include "chord.exception.h"
#include "chord.concurrent.queue.h"

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

void Client::leave() {

  // get successor
  const auto n = router->successor();
  const auto successor = n->uuid;
  const auto endpoint = n->endpoint;

  if(successor == context.uuid()) {
    logger->info("[leaving] no successor - shutting down");
    return;
  }

  ClientContext clientContext;
  LeaveRequest req;
  LeaveResponse res;

  logger->trace("leaving chord ring, informing {}@{}", successor, endpoint);

  req.mutable_header()->CopyFrom(make_header(context));
  const auto status = make_stub(endpoint)->leave(&clientContext, req, &res);

  if (!status.ok()) {
    throw__exception("Failed to inform successor about leave");
  }
}

void Client::join(const endpoint_t &addr) {
  logger->debug("joining {}", addr);

  ClientContext clientContext;
  JoinRequest req;

  req.mutable_header()->CopyFrom(make_header(context));

  JoinResponse res;
  const auto status = make_stub(addr)->join(&clientContext, req, &res);

  if (!status.ok() || !res.has_successor()) {
    throw__exception("Failed to join " + addr);
  }

  const auto entry = res.successor();
  const auto id = entry.uuid();
  const auto endpoint = entry.endpoint();

  logger->trace("successful, received successor {}@{}", id, endpoint);
  router->set_successor(0, uuid_t{id}, endpoint);
}

void Client::take(const node from, const node to, const take_consumer_t callback ) {
  ClientContext clientContext;
  TakeResponse res;
  TakeRequest req;

  req.mutable_header()->CopyFrom(make_header(context));
  req.set_from(from.uuid);
  req.set_to(to.uuid);

  // cannot be mocked since make_stub returns unique_ptr<StubInterface> (!)
  const auto stub = Chord::NewStub(grpc::CreateChannel(from.endpoint, grpc::InsecureChannelCredentials()));
  unique_ptr<ClientReader<TakeResponse> > reader(stub->take(&clientContext, req));

  while (reader->Read(&res)) {
    std::cout << "gonna take: " << res.id();
    //TODO move check upwards, dont do request
    if(callback) {
      callback(res);
    }
  }

  reader->Finish();
}

void Client::take() {
  const auto successor = router->successor();

  if(!successor) {
    logger->warn("[take] failed - no successor");
    return;
  }

  take(context.node(), *successor, take_consumer_callback);
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
    logger->warn("[stabilize] failed - removing endpoint {}@{}?", *successor, endpoint);
    router->reset(*successor);
    return;
  }

  if (res.has_predecessor()) {
    logger->trace("received stabilize response with predecessor {}@{}", res.predecessor().uuid(), res.predecessor().endpoint());
    const RouterEntry &entry = res.predecessor();

    const uuid_t self(context.uuid());
    const uuid_t pred(entry.uuid());
    const uuid_t succ(router->successor()->uuid);
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

  if (!status.ok()) throw__grpc_exception("failed to query succesor", status);

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
  } else if(!res.has_header()) {
    logger->error("[check] returned without header, should remove endpoint {}@{}?", *predecessor, endpoint);
  }
}
}
