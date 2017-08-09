#include <memory>
#include <grpc/grpc.h>

#include <grpc++/channel.h>
#include <grpc++/client_context.h>
#include <grpc++/create_channel.h>
#include <grpc++/security/credentials.h>

#include "chord.log.h"
#include "chord.uuid.h"
#include "chord.client.h"
#include "chord.router.h"
#include "chord.context.h"
#include "chord.grpc.pb.h"


#define log(level) LOG(level) << "[client] "
#define CLIENT_LOG(level, method) LOG(level) << "[client][" << #method << "] "

using namespace std;

ChordClient::ChordClient(Context& context, Router& router) 
  :context{context}
  ,router{router}
{
  //--- default stub factory
  make_stub = [&](const endpoint_t& endpoint) {
    return Chord::NewStub(grpc::CreateChannel(endpoint, grpc::InsecureChannelCredentials()));
  };
}

ChordClient::ChordClient(Context& context, Router& router, StubFactory make_stub) 
  :context{context}
  ,router {router}
  ,make_stub{make_stub}
{
}

Header ChordClient::make_header() {
  ClientContext clientContext;

  Header header;
  RouterEntry src;
  src.set_uuid(to_string(context.uuid()));
  src.set_endpoint(context.bind_addr);
  header.mutable_src()->CopyFrom(src);
  return header;
}

bool ChordClient::join(const endpoint_t& addr) {
  CLIENT_LOG(debug, join) << "joining " << addr;

  ClientContext clientContext;
  JoinRequest req;

  req.mutable_header()->CopyFrom(make_header());

  JoinResponse res;
  Status status = make_stub(addr)->join(&clientContext, req, &res);

  if(!res.has_successor()) {
    CLIENT_LOG(fatal,join) << "failed " << addr;
    return false;
  }

  CLIENT_LOG(trace,join) << "successful, received successor " << res.successor().uuid() << "@" << res.successor().endpoint();
  router.set_successor(0, uuid_t(res.successor().uuid()), res.successor().endpoint());

  return true;

}

void ChordClient::stabilize() {
  ClientContext clientContext;
  StabilizeRequest req;
  StabilizeResponse res;

  req.mutable_header()->CopyFrom(make_header());

  auto successor = router.successor();

  //--- return if join failed or uuid == successor (create)
  if(successor == nullptr ) {
    CLIENT_LOG(trace, stabilize) << "no successor found";
    return;
  } else if( (*successor) == context.uuid() ) {
    CLIENT_LOG(trace, stabilize) << "successor is me, still bootstrapping";
    return;
  }

  endpoint_t endpoint = router.get(successor);

  CLIENT_LOG(trace, stabilize) << "calling stabilize on successor " << endpoint;
  Status status = make_stub(endpoint)->stabilize(&clientContext, req, &res);

  if( !status.ok() ) {
    CLIENT_LOG(warning, stabilize) << "failed - should remove endpoint " << endpoint << "?";
    router.reset(successor);
    return;
  }

  if(res.has_predecessor()) {
    CLIENT_LOG(trace, stabilize) << "received stabilize response with predecessor "
      << res.predecessor().uuid() << "@" << res.predecessor().endpoint();
    RouterEntry entry = res.predecessor();

    //--- validate
    //if( !entry.has_uuid() and !entry.has_entrypoint() ) {
    //  CLIENT_LOG(warn) << "received empty routing entry";
    //  return;
    //}

    uuid_t self(context.uuid());
    uuid_t pred(entry.uuid());
    uuid_t succ(*router.successor());

    if( (pred > self and pred < succ) or
        (pred > self and succ < pred) ) {
      //if(   (pred > self and succ < pred)) {
      router.set_successor(0, pred, entry.endpoint());
    }
    } else {
      CLIENT_LOG(trace, stabilize) << "received empty routing entry";
    }

    notify();
  }

  void ChordClient::notify() {

    // get successor
    auto successor = router.successor();
    endpoint_t endpoint = router.get(successor);

    ClientContext clientContext;
    NotifyRequest req;
    NotifyResponse res;

    CLIENT_LOG(trace, notify) << "calling notify on address " << endpoint;

    req.mutable_header()->CopyFrom(make_header());
    make_stub(endpoint)->notify(&clientContext, req, &res);

  }

  Status ChordClient::successor(ClientContext* clientContext, const SuccessorRequest* req, SuccessorResponse* res) {

    CLIENT_LOG(trace,successor) << "trying to find successor of " << req->id();
    SuccessorRequest copy(*req);
    copy.mutable_header()->CopyFrom(make_header());

    uuid_t predecessor = router.closest_preceding_node(uuid_t(req->id()));
    endpoint_t endpoint = router.get(predecessor);
    CLIENT_LOG(trace,successor) << "forwarding request to " << endpoint;

    return make_stub(endpoint)->successor(clientContext, copy, res);

  }

  /** called by chord.service **/
  Status ChordClient::successor(const SuccessorRequest* req, SuccessorResponse* res) {
    ClientContext clientContext;
    return successor(&clientContext, req, res);
  }

  void ChordClient::check() {
    auto predecessor = router.predecessor();
    auto successor = router.successor();

    if( predecessor == nullptr ) {
      CLIENT_LOG(trace, check) << "no predecessor, skip.";
      return;
    }
    if( successor == nullptr ) {
      CLIENT_LOG(trace, check) << "no successor, skip.";
      return;
    }

    ClientContext clientContext;
    CheckRequest req;
    CheckResponse res;

    req.mutable_header()->CopyFrom(make_header());

    auto endpoint = router.get(predecessor);

    CLIENT_LOG(trace, check) << "checking predecessor " << *predecessor << "@" << endpoint;
    const grpc::Status status = make_stub(endpoint)->check(&clientContext, req, &res);

    if( !status.ok() ) {
      CLIENT_LOG(warning, check) << "predecessor failed.";
      router.reset_predecessor(0);
    }
    if( !res.has_header() ) {
      cerr << "CHECK RETURNED WITHOUT HEADER! SHOULD REMOVE " << endpoint << " ?";
    }
  }

