#pragma once

#include <memory>
#include <functional>

#include "chord.types.h"
#include "chord_controller.grpc.pb.h"

typedef std::function<std::shared_ptr<chord::controller::Control::StubInterface>(
    const endpoint_t &endpoint)> ControlStubFactory;

namespace chord {
namespace controller {
class Client {
 private:
  ControlStubFactory make_stub;

 public:
  Client();

  explicit Client(ControlStubFactory factory);

  void control(const std::string &command);

};
} //namespace controller
} //namespace chord
