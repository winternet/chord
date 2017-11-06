#include <memory>
#include <grpc/grpc.h>
#include <fstream>

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
#include "chord.exception.h"

#include "chord.crypto.h"

#define log(level) LOG(level) << "[client] "
#define CLIENT_LOG(level, method) LOG(level) << "[client][" << #method << "] "

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using grpc::ClientWriter;
using grpc::ClientReader;

using chord::Header;
using chord::RouterEntry;

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
using chord::PutResponse;
using chord::PutRequest;
using chord::GetResponse;
using chord::GetRequest;

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
  src.set_uuid(context.uuid());
  src.set_endpoint(context.bind_addr);
  header.mutable_src()->CopyFrom(src);
  return header;
}

void ChordClient::join(const endpoint_t& addr) {
  CLIENT_LOG(debug, join) << "joining " << addr;

  ClientContext clientContext;
  JoinRequest req;

  req.mutable_header()->CopyFrom(make_header());

  JoinResponse res;
  Status status = make_stub(addr)->join(&clientContext, req, &res);

  if(!status.ok() || !res.has_successor()) {
    throw chord::exception("Failed to join " + addr);
  }

  auto entry = res.successor();
  auto id = entry.uuid();
  auto endpoint = entry.endpoint();

  CLIENT_LOG(trace, join) << "successful, received successor " << id << "@" << endpoint;
  router.set_successor(0, uuid_t{id}, endpoint);
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

RouterEntry ChordClient::successor(const uuid_t& uuid) {
  ClientContext clientContext;
  SuccessorRequest req;
  req.mutable_header()->CopyFrom(make_header());
  req.set_id(uuid);
  SuccessorResponse res;

  auto status = successor(&clientContext, &req, &res);

  if(!status.ok()) throw chord::exception("failed to query succesor", status);

  return res.successor();
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

Status ChordClient::put(const std::string& uri, std::istream& istream) {
  auto hash = chord::crypto::sha256(uri);
  auto endpoint = successor(hash).endpoint();

  CLIENT_LOG(trace, put) << uri << " (" << hash << ")";

  //TODO make configurable
  constexpr size_t len = 512 * 1024; // 512k
  char buffer[len];
  size_t offset = 0, 
         read = 0;

  ClientContext clientContext;
  PutResponse res;
  // cannot be mocked since make_stub returns unique_ptr<StubInterface> (!)
  auto stub = Chord::NewStub(grpc::CreateChannel(endpoint, grpc::InsecureChannelCredentials()));
  std::unique_ptr<ClientWriter<PutRequest> > writer(stub->put(&clientContext, &res));

  do {
    read = istream.readsome(buffer, len);
    if(read <= 0) {
      break;
    }

    PutRequest req;
    req.set_id(hash);
    req.set_data(buffer, read);
    req.set_offset(offset);
    req.set_size(read);
    req.set_uri(uri);
    offset += read;

    if(!writer->Write(req)) {
      throw chord::exception("broken stream.");
    }
    
  } while(read > 0);

  writer->WritesDone();
  return writer->Finish();
}

Status ChordClient::get(const std::string& uri, std::ostream& ostream) {
  auto hash = chord::crypto::sha256(uri);
  auto endpoint = successor(hash).endpoint();

  CLIENT_LOG(trace, get) << uri << " (" << hash << ")";

  ClientContext clientContext;
  GetResponse res;
  GetRequest req;

  req.set_id(hash);
  req.set_uri(uri);

  // cannot be mocked since make_stub returns unique_ptr<StubInterface> (!)
  auto stub = Chord::NewStub(grpc::CreateChannel(endpoint, grpc::InsecureChannelCredentials()));
  unique_ptr<ClientReader<GetResponse> > reader(stub->get(&clientContext, req));

  while(reader->Read(&res)) {
    ostream.write(res.data().data(), res.size());
  }

  return reader->Finish();
}
