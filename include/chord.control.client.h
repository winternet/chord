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

using chord::ChordControl;
using chord::ControlResponse;
using chord::ControlRequest;

typedef std::function<std::shared_ptr<chord::ChordControl::StubInterface>(const endpoint_t& endpoint)> ControlStubFactory;

class ChordControlClient {
private:
  const std::shared_ptr<Context>& context;

  ControlStubFactory make_stub;

  /**
   * return new header
   */
  //Header make_header();
  //SuccessorRequest make_request();

public:
  ChordControlClient(const std::shared_ptr<Context>& context);
  ChordControlClient(const std::shared_ptr<Context>& context, ControlStubFactory factory);

  void control(const std::string& command);
  
};
