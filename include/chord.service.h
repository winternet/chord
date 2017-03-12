#pragma once

#include <grpc/grpc.h>

#include <grpc++/server.h>
#include <grpc++/server_builder.h>
#include <grpc++/server_context.h>
#include <grpc++/security/server_credentials.h>

#include "chord.context.h"
#include "chord.grpc.pb.h"

#include "chord.client.h"

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
using chord::RouterEntry;
using chord::Chord;

class ChordServiceImpl final : public Chord::Service {

public:
  ChordServiceImpl(std::shared_ptr<Context> context);

  Status join(ServerContext* context, const JoinRequest* req, JoinResponse* res);

  Status successor(ServerContext* context, const SuccessorRequest* req, SuccessorResponse* res);

  Status stabilize(ServerContext* context, const StabilizeRequest* req, StabilizeResponse* res);

  Status notify(ServerContext* context, const NotifyRequest* req, NotifyResponse* res);

  //void start_server(const std::string& addr) {
  //  ChordServiceImpl serviceImpl;

  //  ServerBuilder builder;
  //  builder.AddListeningPort(addr, grpc::InsecureServerCredentials());
  //  builder.RegisterService(&serviceImpl);

  //  std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
  //  BOOST_LOG_TRIVIAL(debug) << "server listening on " << addr;
  //  server->Wait();
  //}

private:
  std::shared_ptr<Context> context { nullptr };
  std::shared_ptr<Router> router { nullptr };
};
