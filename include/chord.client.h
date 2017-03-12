#pragma once

#include <memory>

#include "chord.log.h"
#include "chord.uuid.h"
#include "chord.router.h"
#include "chord.context.h"
#include "chord.grpc.pb.h"

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;

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
using chord::Chord;

class ChordClient {
private:
  std::shared_ptr<Context> context { nullptr };

  /**
   * return new stub to be used within the client.
   */
  static std::unique_ptr<Chord::Stub> make_stub(const endpoint_t& endpoint);

  /**
   * return new header
   */
  Header make_header();

public:
  ChordClient(const std::shared_ptr<Context>& context);

  void join(const endpoint_t& addr);

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


  void connect(const endpoint_t& addr);
  
private:
  std::unique_ptr<Chord::Stub> stub;
  uuid_t uuid;
};
