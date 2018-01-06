#pragma once

#include <grpc++/server_context.h>

#include "chord_controller.grpc.pb.h"

namespace chord {
namespace fs {
class Facade;
}  // namespace fs
}  // namespace chord

namespace chord {
namespace controller {
class Service final : public chord::controller::Control::Service {
 public:
  explicit Service(chord::fs::Facade *filesystem);

  grpc::Status control(grpc::ServerContext *context,
                       const chord::controller::ControlRequest *req,
                       chord::controller::ControlResponse *res) override;

 private:
  grpc::Status parse_command(const chord::controller::ControlRequest *req,
                             chord::controller::ControlResponse *res);

  chord::fs::Facade *filesystem;
};
}  // namespace controller
}  // namespace chord
