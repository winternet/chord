#include <grpc/grpc.h>

#include <grpc++/server.h>
#include <grpc++/server_builder.h>
#include <grpc++/server_context.h>
#include <grpc++/security/server_credentials.h>


#include "chord.log.h"
#include "chord.context.h"
#include "chord.grpc.pb.h"

#include "chord.client.h"
#include "chord.service.h"

#define log(level) LOG(level) << "[service] "

#define SERVICE_LOG(level,method) LOG(level) << "[service][" << #method << "] "

using grpc::ServerContext;
using grpc::ServerBuilder;
using grpc::Status;

using chord::JoinResponse;
using chord::JoinRequest;
using chord::SuccessorResponse;
using chord::SuccessorRequest;
using chord::StabilizeResponse;
using chord::StabilizeRequest;
using chord::NotifyResponse;
using chord::NotifyRequest;
using chord::Chord;

ChordServiceImpl::ChordServiceImpl(std::shared_ptr<Context> context)
 : context{ context }
 , router{ context->router }
{
  make_client = [&](){
    return ChordClient(context);
  };
}

ChordServiceImpl::ChordServiceImpl(std::shared_ptr<Context> context, ClientFactory make_client)
 : context{ context }
 , router{ context->router }
 , make_client{ make_client }
{}

Status ChordServiceImpl::join(ServerContext* serverContext, const JoinRequest* req, JoinResponse* res) {
  SERVICE_LOG(trace, join) << "from " << req->header().src().uuid()
    << "@" << req->header().src().endpoint();
  /**
   * find successor
   */
  Header header;
  RouterEntry src;
  header.mutable_src()->CopyFrom(req->header().src());

  SuccessorRequest succReq;
  succReq.mutable_header()->CopyFrom(header);
  succReq.set_id(req->header().src().uuid());
  SuccessorResponse succRes;

  successor(serverContext, &succReq, &succRes);

  res->mutable_successor()->CopyFrom(succRes.successor());

  /**
   * initialize the ring
   */
  if( router->successor() != nullptr and
      *router->successor() == context->uuid() ) {
    SERVICE_LOG(info,join) << "first node joining, setting the node as successor";
    router->set_successor(0, uuid_t(req->header().src().uuid()), req->header().src().endpoint());
  }

  return Status::OK;
}

Header ChordServiceImpl::make_header() {
  ClientContext clientContext;

  Header header;
  RouterEntry src;
  src.set_uuid(to_string(context->uuid()));
  src.set_endpoint(context->bind_addr);
  header.mutable_src()->CopyFrom(src);
  return header;
}

Status ChordServiceImpl::successor(ServerContext* serverContext, const SuccessorRequest* req, SuccessorResponse* res) {
  SERVICE_LOG(trace,successor) << "from " << req->header().src().uuid()
    << "@" << req->header().src().endpoint()
    << " successor of? " << req->id();

  //--- destination of the request
  uuid_t id( req->id() );
  uuid_t self( context->uuid() );
  uuid_t successor( *router->successor() );

  std::cerr << "\t****++++++ get successor: " << router->get(successor);
  std::cerr << "\t****++++++ is empty? " << router->get(successor).empty();
  if( router->get(successor).empty() ) {
    router->reset(successor);
    return Status::CANCELLED;
  }

  if( id > self and (id <= successor or (successor < self and id > successor)) ) {
    SERVICE_LOG(trace,successor) << "of " << id << " is " << to_string(successor);
    //--- router entry
    RouterEntry entry;
    entry.set_uuid(to_string(successor));
    entry.set_endpoint(router->get(successor));

    res->mutable_successor()->CopyFrom(entry);
  } else {
    SERVICE_LOG(trace, successor) << "id not within self(" << context->uuid() << ") <--> successor(" << *router->successor() << ") trying to forward.";
    uuid_t next = router->closest_preceding_node(id);
    SERVICE_LOG(trace, successor) << "CLOSEST PRECEDING NODE: " << next;
    if( next == self ) {
      RouterEntry entry;
      entry.set_uuid(to_string(self));
      entry.set_endpoint(router->get(self));
      
      //SERVICE_LOG(trace,successor) << "ENDPOINT: " << entry.endpoint();
      res->mutable_successor()->CopyFrom(entry);
    } else {
      SERVICE_LOG(trace, successor) << "trying to spawn client to forward request";
      Status status = make_client().successor(req, res);
      if( !status.ok() ) {
        SERVICE_LOG(warning, successor) << "failed to query successor from client!";
      }
      SERVICE_LOG(trace, successor) << "spawned client, got result";
    }
  }

  return Status::OK;
}

Status ChordServiceImpl::stabilize(ServerContext* serverContext, const StabilizeRequest* req, StabilizeResponse* res) {
  SERVICE_LOG(trace,stabilize) << "from " << req->header().src().uuid()
    << "@" << req->header().src().endpoint();
 
  std::shared_ptr<uuid_t> predecessor = router->predecessor();
  if(predecessor != nullptr) {
    SERVICE_LOG(debug,stabilize) << "returning predecessor " << *predecessor;
    endpoint_t endpoint = router->get(predecessor);

    RouterEntry entry;
    entry.set_uuid(to_string(*predecessor));
    entry.set_endpoint(endpoint);

    res->mutable_predecessor()->CopyFrom(entry);
  }

  return Status::OK;
}

Status ChordServiceImpl::notify(ServerContext* serverContext, const NotifyRequest* req, NotifyResponse* res) {
  SERVICE_LOG(trace,notify) << "from " << req->header().src().uuid()
    << "@" << req->header().src().endpoint();

  std::shared_ptr<uuid_t> predecessor = router->predecessor();

  //--- validate
  if(!req->has_header() && !req->header().has_src()) {
    log(warning) << "failed to validate header";
    return Status::CANCELLED;
  }

  uuid_t self( context->uuid() );
  uuid_t uuid = uuid_t(req->header().src().uuid());
  endpoint_t endpoint = req->header().src().endpoint();

  if( predecessor == nullptr or 
      (uuid < *predecessor and uuid < self) or
      (uuid > *predecessor and uuid > self) ) {
    router->set_predecessor( 0, uuid, endpoint );
  } else {
    if( predecessor == nullptr )
      log(info) << "predecessor is null";
    else {
      log(info) << "n' = " << uuid << "\npredecessor = " << *predecessor << "\nself = " << self;
    }
  }

  return Status::OK;
}

Status ChordServiceImpl::check(ServerContext* serverContext, const CheckRequest* req, CheckResponse* res) {
  SERVICE_LOG(trace,check) << "from " << req->header().src().uuid()
    << "@" << req->header().src().endpoint();
  return Status::OK;
}

void ChordServiceImpl::fix_fingers(size_t index) {
  //if( router->successor() == nullptr ) {
  //  SERVICE_LOG(trace, fix_fingers) << "fixing fingers for direct successor.";
  //  index = 1;
  //}
  uuid_t fix = context->uuid() + uuid_t(std::pow(2., (double)index-1));

  SERVICE_LOG(trace, fix_fingers) << "fixing finger for " << to_string(fix) << ".";

  ServerContext serverContext;
  SuccessorRequest req;
  SuccessorResponse res;

  req.mutable_header()->CopyFrom(make_header());
  req.set_id(to_string(fix));

  Status status = successor(&serverContext, &req, &res);
  if( status.ok() ) {
    SERVICE_LOG(trace, fix_fingers) << "\t\t *** received response: " << res.successor().uuid() 
      << "@" << res.successor().endpoint();

    router->set_successor(index, uuid_t(res.successor().uuid()), res.successor().endpoint());
  } else {
    SERVICE_LOG(warning, fix_fingers) << "failed to fix fingers for " << fix;
  }
}
