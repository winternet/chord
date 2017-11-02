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
#include "chord.exception.h"
#include "chord.control.client.h"


#define log(level) LOG(level) << "[client] "
#define CLIENT_LOG(level, method) LOG(level) << "[client][" << #method << "] "

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;

using chord::Header;
using chord::RouterEntry;

using chord::ChordControl;
using chord::ControlResponse;
using chord::ControlRequest;

using namespace std;

ChordControlClient::ChordControlClient() 
{
  //--- default stub factory
  make_stub = [&](const endpoint_t& endpoint) {
    return ChordControl::NewStub(grpc::CreateChannel(endpoint, grpc::InsecureChannelCredentials()));
  };
}

ChordControlClient::ChordControlClient(ControlStubFactory make_stub) 
  :make_stub{make_stub}
{
}

void ChordControlClient::control(const string& command) {
  ClientContext clientContext;
  ControlRequest req;
  ControlResponse res;

  req.set_command(command);
  //TODO make configurable (at least port)
  auto status = make_stub("127.0.0.1:50000")->control(&clientContext, req, &res);
  if(status.ok()) {
    return;
  }

  throw chord::exception("failed to issue command: " + command);
}
/*
   Header ChordControlClient::make_header() {
   ClientContext clientContext;

   Header header;
   RouterEntry src;
   src.set_uuid(context.uuid());
   src.set_endpoint(context.bind_addr);
   header.mutable_src()->CopyFrom(src);
   return header;
   }

   void ChordControlClient::join(const endpoint_t& addr) {
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

   void ChordControlClient::stabilize() {
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

void ChordControlClient::notify() {

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

*/
