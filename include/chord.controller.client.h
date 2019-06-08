#pragma once

#include <functional>
#include <memory>

#include "chord.types.h"
#include "chord_controller.grpc.pb.h"

using ControlStubFactory =
    std::function<std::shared_ptr<chord::controller::Control::StubInterface>(
        const chord::endpoint& endpoint)>;

namespace chord {
namespace controller {
class Client {
 private:
  ControlStubFactory make_stub;
  static constexpr auto logger_name = "chord.controller.client";

 public:
  Client();

  explicit Client(ControlStubFactory factory);

  void control(const std::string &command);

};
} //namespace controller
} //namespace chord
