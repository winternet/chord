#pragma once

#include <memory>
#include <functional>

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
using chord::CheckResponse;
using chord::CheckRequest;
using chord::Chord;

typedef std::function<std::shared_ptr<chord::Chord::StubInterface>(const endpoint_t& endpoint)> StubFactory;

class ChordClient {
private:
  std::shared_ptr<Context> context { nullptr };
  std::shared_ptr<Router> router { nullptr };
  StubFactory make_stub;

  /**
   * return new header
   */
  Header make_header();

public:
  ChordClient(const std::shared_ptr<Context>& context);
  ChordClient(const std::shared_ptr<Context>& context, StubFactory factory);

  bool join(const endpoint_t& addr);

  void stabilize();
  void notify();
  void check();
  void fix_fingers();

  Status successor(ClientContext* context, const SuccessorRequest* req, SuccessorResponse* res);
  Status successor(const SuccessorRequest* req, SuccessorResponse* res);
  

  /*
  Status stabilize(ClientContext* context, const StabilizeRequest* req, StabilizeResponse* res);
  void stabilize(const StabilizeRequest* req, StabilizeResponse* res);
  */
//
//  void notify(ClientContext* context, const NotifyRequest* req, NotifyResponse* res) {
//    BOOST_LOG_TRIVIAL(debug) << "received notification " << req;
//    return Status::OK;
//  }
};
