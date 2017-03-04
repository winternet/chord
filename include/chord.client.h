#pragma once

#include <memory>
#include <grpc/grpc.h>

#include <grpc++/channel.h>
#include <grpc++/client_context.h>
#include <grpc++/create_channel.h>
#include <grpc++/security/credentials.h>

#include "chord.log.h"
#include "chord.uuid.h"
#include "chord.context.h"
#include "chord.grpc.pb.h"

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;

using chord::Header;

using chord::JoinResponse;
using chord::JoinRequest;
using chord::SuccessorResponse;
using chord::SuccessorRequest;
using chord::StabilizeResponse;
using chord::StabilizeRequest;
using chord::NotifyResponse;
using chord::NotifyRequest;
using chord::Chord;

class ChordClient {
private:
  std::shared_ptr<Context> context { nullptr };

public:
  ChordClient(std::shared_ptr<Context> context);

  void join(const std::string& addr);

  void successor(ClientContext* context, const SuccessorRequest* req, SuccessorResponse* res);
//  
//  void stabilize(ServerContext* context, const StabilizeRequest* req, StabilizeResponse* res) {
//    BOOST_LOG_TRIVIAL(debug) << "received stabilize request " << req;
//    return Status::OK;
//  }
//
//  void notify(ServerContext* context, const NotifyRequest* req, NotifyResponse* res) {
//    BOOST_LOG_TRIVIAL(debug) << "received notification " << req;
//    return Status::OK;
//  }


  void connect(const std::string& addr);
  
private:
  std::unique_ptr<Chord::Stub> stub;
  uuid_t uuid;
};
