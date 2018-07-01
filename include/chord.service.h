#pragma once

#include <grpc++/server_context.h>
#include <grpc/grpc.h>
#include <functional>

#include "chord.client.h"
#include "chord.exception.h"
#include "chord.grpc.pb.h"
#include "chord.i.service.h"
#include "chord.pb.h"

namespace chord {
struct Context;
struct Router;
}  // namespace chord

namespace spdlog {
  class logger;
}

namespace chord {
using ClientFactory = std::function<chord::Client()>;

class Service final : public chord::Chord::Service, AbstractService {
  static constexpr auto logger_name = "chord.service";

 public:
  Service(Context &context, Router *router);

  Service(Context &context, Router *router, ClientFactory make_client);

  grpc::Status join(grpc::ServerContext *context, const chord::JoinRequest *req,
                    chord::JoinResponse *res) override;

  chord::common::RouterEntry successor(const uuid_t &uuid) noexcept(
      false) override;

  grpc::Status take(grpc::ServerContext *context, 
                    const chord::TakeRequest *req,
                    grpc::ServerWriter<chord::TakeResponse> *writer) override;

  grpc::Status successor(grpc::ServerContext *context,
                         const chord::SuccessorRequest *req,
                         chord::SuccessorResponse *res) override;

  grpc::Status stabilize(grpc::ServerContext *context,
                         const chord::StabilizeRequest *req,
                         chord::StabilizeResponse *res) override;

  grpc::Status notify(grpc::ServerContext *context,
                      const chord::NotifyRequest *req,
                      chord::NotifyResponse *res) override;

  grpc::Status check(grpc::ServerContext *context,
                     const chord::CheckRequest *req,
                     chord::CheckResponse *res) override;

  void fix_fingers(size_t index);

 private:
  Context &context;
  Router *router;
  ClientFactory make_client;
  std::shared_ptr<spdlog::logger> logger;
};
}  // namespace chord
