#pragma once

#include <grpc++/server_context.h>
#include <grpc/grpc.h>
#include <functional>
#include <vector>

#include "chord.signal.h"
#include "chord.exception.h"
#include "chord.grpc.pb.h"
#include "chord.i.callback.h"
#include "chord.i.service.h"
#include "chord.i.client.h"
#include "chord.pb.h"

namespace chord {
struct Context;
struct Router;
class IClient;
}  // namespace chord

namespace spdlog {
  class logger;
}

namespace chord {
using ClientFactory = std::function<chord::IClient*()>;

class Service final : public chord::Chord::Service, public IService {
  static constexpr auto logger_name = "chord.service";

 public:
  Service(Context &context, Router *router, IClient* client);

  grpc::Status join(grpc::ServerContext *context, const chord::JoinRequest *req,
                    chord::JoinResponse *res) override;

  chord::common::RouterEntry successor(const uuid_t &uuid) override;

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

  grpc::Status leave(grpc::ServerContext *context, 
                     const chord::LeaveRequest *req,
                     chord::LeaveResponse *res) override;

  void fix_fingers(size_t index) override;

  signal<void(const node, const node)>& on_leave() override {
    return event_leave;
  }

  ::grpc::Service* grpc_service() override {
    return this;
  }
 private:
  Context &context;
  Router *router;
  IClient *client;
  signal<void(const node, const node)> event_leave;
  std::shared_ptr<spdlog::logger> logger;
};
}  // namespace chord
