#pragma once
#include <memory>
#include <string>
#include <vector>

#include <grpcpp/impl/codegen/status.h>
#include <grpcpp/server_context.h>

#include "chord.path.h"
#include "chord_controller.grpc.pb.h"

namespace chord { class uri; }  // lines 10-10
namespace chord { namespace controller { class ControlRequest; } }
namespace chord { namespace controller { class ControlResponse; } }
namespace chord { namespace fs { class IFacade; } }
namespace chord { struct Context; }
namespace spdlog { class logger; }

namespace chord {
namespace controller {
class Service final : public chord::controller::Control::Service {
  static constexpr auto logger_name = "chord.controller.service";

 public:
  explicit Service(Context& context, chord::fs::IFacade *filesystem);

  grpc::Status control(grpc::ServerContext *context,
                       const chord::controller::ControlRequest *req,
                       chord::controller::ControlResponse *res) override;

 private:
  grpc::Status parse_command(const chord::controller::ControlRequest *req,
                             chord::controller::ControlResponse *res);
  grpc::Status handle_put(const std::vector<std::string>& token, ControlResponse* res);
  grpc::Status handle_get(const std::vector<std::string>& token, ControlResponse* res);
  grpc::Status handle_dir(const std::vector<std::string>& token, ControlResponse* res);
  grpc::Status handle_del(const std::vector<std::string>& token, ControlResponse* res);

  grpc::Status send_file(const path, const chord::uri);

  const Context& context;
  chord::fs::IFacade *filesystem;

  std::shared_ptr<spdlog::logger> logger;
};
}  // namespace controller
}  // namespace chord
