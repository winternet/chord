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


ChordClient::ChordClient(const std::shared_ptr<Context>& context) 
  :context{context}
  ,router{context->router}
{
}

std::unique_ptr<Chord::Stub> ChordClient::make_stub(const endpoint_t& endpoint) {
  return Chord::NewStub(grpc::CreateChannel(endpoint, grpc::InsecureChannelCredentials()));
}

Header ChordClient::make_header() {
  ClientContext clientContext;

  Header header;
  RouterEntry src;
  src.set_uuid(to_string(context->uuid));
  src.set_endpoint(context->bind_addr);
  header.mutable_src()->CopyFrom(src);
  return header;
}

bool ChordClient::join(const endpoint_t& addr) {
  LOG(debug) << "joining " << addr;

  ClientContext clientContext;
  JoinRequest req;

  req.mutable_header()->CopyFrom(make_header());

  JoinResponse res;
  grpc::Status status = make_stub(addr)->join(&clientContext, req, &res);

  LOG(trace) << "joined " << addr << " received successor " << res.successor().uuid() << " at endpoint " << res.successor().endpoint();
  router->set_successor(0, uuid_t(res.successor().uuid()), res.successor().endpoint());
  return true;

  //--- find successor
  //context->successor = 
}

void ChordClient::stabilize() {
  ClientContext clientContext;
  StabilizeRequest req;

  req.mutable_header()->CopyFrom(make_header());

  StabilizeResponse res;
  uuid_t successor = router->successor();
  endpoint_t endpoint = router->get(successor);

  LOG(trace) << "calling stabilize on address " << endpoint;
  make_stub(endpoint)->stabilize(&clientContext, req, &res);
}

Status ChordClient::successor(ClientContext* context, const SuccessorRequest* req, SuccessorResponse* res) {

}

void ChordClient::successor(const SuccessorRequest* req, SuccessorResponse* res) {
  ClientContext *context;
  successor(context, req, res);
}

Status ChordClient::stabilize(ClientContext* context, const StabilizeRequest* req, StabilizeResponse* res) {
}

void ChordClient::stabilize(const StabilizeRequest* req, StabilizeResponse* res) {
  ClientContext *context;
  stabilize(context, req, res);
}
//  
//  void ChordClient::stabilize(ServerContext* context, const StabilizeRequest* req, StabilizeResponse* res) {
//    BOOST_LOG_TRIVIAL(debug) << "received stabilize request " << req;
//    return Status::OK;
//  }
//
//  void notify(ServerContext* context, const NotifyRequest* req, NotifyResponse* res) {
//    BOOST_LOG_TRIVIAL(debug) << "received notification " << req;
//    return Status::OK;
//  }


void ChordClient::connect(const std::string& addr) {
  LOG(debug) << "Connecting to " << addr;

  stub = Chord::NewStub(grpc::CreateChannel(addr, grpc::InsecureChannelCredentials()));

  //LOG(trace) << "Client has uuid " << uuid;

  join(addr);
}
