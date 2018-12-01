#pragma once

#include <grpc++/server_context.h>
#include <grpc/grpc.h>
#include <functional>
#include <vector>

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

class Service final : public chord::Chord::Service, IService {
  static constexpr auto logger_name = "chord.service";

 public:
  Service(Context &context, Router *router, IClient* client);

  grpc::Status join(grpc::ServerContext *context, const chord::JoinRequest *req,
                    chord::JoinResponse *res) override;

  chord::common::RouterEntry successor(const uuid_t &uuid) override;

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

  grpc::Status leave(grpc::ServerContext *context, 
                     const chord::LeaveRequest *req,
                     chord::LeaveResponse *res) override;

  void fix_fingers(size_t index);

  //TODO move to cc
  void set_take_callback(const take_producer_t callback) {
    take_producer_callback = callback;
  }

  //TODO move to cc
  void set_on_leave_callback(const take_consumer_t callback) {
    on_leave_callback = callback;
  }

 private:
  Context &context;
  Router *router;
  IClient *client;
  std::shared_ptr<spdlog::logger> logger;
  // callbacks
  take_producer_t take_producer_callback;
  take_consumer_t on_leave_callback;
};
}  // namespace chord
