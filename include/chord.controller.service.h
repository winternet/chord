#pragma once
#include <functional>

#include <grpc/grpc.h>

#include <grpc++/server.h>
#include <grpc++/server_builder.h>
#include <grpc++/server_context.h>
#include <grpc++/security/server_credentials.h>

#include "chord_controller.grpc.pb.h"

#include "chord.fs.client.h"


namespace chord {
  namespace controller {
    class Service final : public chord::controller::Control::Service {

      public:
        Service(const std::shared_ptr<chord::fs::Client>& fs_client);

        grpc::Status control(grpc::ServerContext* context, const chord::controller::ControlRequest* req, chord::controller::ControlResponse* res) override;

      private:
        grpc::Status parse_command(const chord::controller::ControlRequest* req, chord::controller::ControlResponse* res);

        const std::shared_ptr<chord::fs::Client>& fs_client;
    };
  }
}
