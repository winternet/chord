#include <grpc/grpc.h>

#include <grpc++/server.h>
#include <grpc++/server_builder.h>
#include <grpc++/server_context.h>
#include <grpc++/security/server_credentials.h>

#include <boost/log/trivial.hpp>

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

ChordServiceImpl::ChordServiceImpl(std::shared_ptr<Context> context) :
  context{ context }
{}

Status ChordServiceImpl::join(ServerContext* context, const JoinRequest* req, JoinResponse* res) {
  BOOST_LOG_TRIVIAL(debug) << "Received join request";

  return Status::OK;
}

Status ChordServiceImpl::successor(ServerContext* context, const SuccessorRequest* req, SuccessorResponse* res) {
  BOOST_LOG_TRIVIAL(debug) << "received find successor request";

  //if( context.uuid > 
  return Status::OK;
}

Status ChordServiceImpl::stabilize(ServerContext* context, const StabilizeRequest* req, StabilizeResponse* res) {
  BOOST_LOG_TRIVIAL(debug) << "received stabilize request";
  return Status::OK;
}

Status ChordServiceImpl::notify(ServerContext* context, const NotifyRequest* req, NotifyResponse* res) {
  BOOST_LOG_TRIVIAL(debug) << "received notification";
  return Status::OK;
}

//void start_server(const std::string& addr) {
//  ChordServiceImpl serviceImpl;

//  ServerBuilder builder;
//  builder.AddListeningPort(addr, grpc::InsecureServerCredentials());
//  builder.RegisterService(&serviceImpl);

//  std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
//  BOOST_LOG_TRIVIAL(debug) << "server listening on " << addr;
//  server->Wait();
//}
