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

Status ChordServiceImpl::successor(ServerContext* serverContext, const SuccessorRequest* req, SuccessorResponse* res) {
  SERVICE_LOG(trace,successor) << "from " << req->header().src().uuid()
    << "@" << req->header().src().endpoint()
    << " successor of? " << req->id();

  //--- destination of the request
  uuid_t id( req->id() );
  uuid_t self( context->uuid() );
  uuid_t successor( *router->successor() );

  if( id > self and id <= successor ) {
    SERVICE_LOG(trace,successor) << "of " << id << " is " << to_string(successor);
    //--- router entry
    RouterEntry entry;
    entry.set_uuid(to_string(successor));
    entry.set_endpoint(router->get(successor));

    res->mutable_successor()->CopyFrom(entry);
  } else {
    uuid_t next = router->closest_preceding_node(id);
    SERVICE_LOG(trace, successor) << "CLOSEST PRECEDING NODE: " << next;
    if( next == self ) {
      RouterEntry entry;
      entry.set_uuid(to_string(self));
      entry.set_endpoint(router->get(self));
      
      SERVICE_LOG(trace,successor) << "ENDPOINT: " << entry.endpoint();
      res->mutable_successor()->CopyFrom(entry);
    } else {
      ChordClient client(context);
      SuccessorRequest  fwdReq;//(*req);
      SuccessorResponse fwdRes;//(*res);
      client.successor(&fwdReq, &fwdRes);
      //res->CopyFrom(fwdRes);
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
