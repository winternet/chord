#pragma once

#include <grpc++/server_context.h>

#include "chord_controller.grpc.pb.h"

namespace chord {
  namespace fs {
    class Client;
  }
}

namespace chord {
  namespace controller {
    class Service final : public chord::controller::Control::Service {

      public:
        Service(std::shared_ptr<chord::fs::Client> fs_client);

        grpc::Status control(grpc::ServerContext* context, const chord::controller::ControlRequest* req, chord::controller::ControlResponse* res) override;

      private:
        grpc::Status parse_command(const chord::controller::ControlRequest* req, chord::controller::ControlResponse* res);

        std::shared_ptr<chord::fs::Client> fs_client;
    };
  }
}
