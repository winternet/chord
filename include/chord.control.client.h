#pragma once

#include <memory>
#include <functional>

#include "chord.log.h"
#include "chord.uuid.h"
#include "chord.router.h"
#include "chord.grpc.pb.h"


typedef std::function<std::shared_ptr<chord::ChordControl::StubInterface>(const endpoint_t& endpoint)> ControlStubFactory;

class ChordControlClient {
private:
  ControlStubFactory make_stub;

  /**
   * return new header
   */
  //Header make_header();
  //SuccessorRequest make_request();

public:
  ChordControlClient();
  ChordControlClient(ControlStubFactory factory);

  void control(const std::string& command);
  
};
