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
  LOG(debug) << "received join request from client-id " << req->header().src().uuid()
    << " endpoint " << req->header().src().endpoint();
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

  // return successor
  res->mutable_successor()->CopyFrom(succRes.successor());

  return Status::OK;
}

Status ChordServiceImpl::successor(ServerContext* serverContext, const SuccessorRequest* req, SuccessorResponse* res) {
  LOG(debug) << "received find successor request from client-id " << req->header().src().uuid()
    << " endpoint " << req->header().src().endpoint()
    << " looking for successor of " << req->id();

  //--- destination of the request
  uuid_t id( req->id() );
  uuid_t self( context->uuid );
  uuid_t successor( router->successor() );

  if( self > id and id <= successor ) {
    LOG(trace) << "successor of " << id << " is " << to_string(successor);
    //--- router entry
    RouterEntry entry;
    entry.set_uuid(to_string(successor));
    LOG(trace) << "setting successor " << router->get(successor);
    entry.set_endpoint(router->get(successor));

    res->mutable_successor()->CopyFrom(entry);
  } else {
    uuid_t next = router->closest_preceding_node(id);
    std::cerr << "next node " << to_string(next) << " self: " << to_string(self) << std::endl;
    if( next == self ) {
      RouterEntry entry;
      entry.set_uuid(to_string(self));
      entry.set_endpoint(router->get(self));
      
      res->mutable_successor()->CopyFrom(entry);
    } else {
      ChordClient client(context);
      client.successor(req, res);
    }
  }

  return Status::OK;
}

Status ChordServiceImpl::stabilize(ServerContext* context, const StabilizeRequest* req, StabilizeResponse* res) {
  LOG(debug) << "received stabilize request";

  return Status::OK;
}

Status ChordServiceImpl::notify(ServerContext* context, const NotifyRequest* req, NotifyResponse* res) {
  LOG(debug) << "received notification";
  return Status::OK;
}
