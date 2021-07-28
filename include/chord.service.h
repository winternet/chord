#pragma once
#include "chord_common.pb.h"

#include <cstddef>
#include <functional>                    // for function
#include <memory>

#include <grpcpp/impl/codegen/status.h>
#include <grpcpp/impl/codegen/service_type.h>

#include <grpcpp/server_context.h>

#include "chord.grpc.pb.h"
#include "chord.i.service.h"
#include "chord.uuid.h"
#include "chord.signal.h"

namespace chord { class CheckRequest; }
namespace chord { class CheckResponse; }
namespace chord { class JoinRequest; }
namespace chord { class JoinResponse; }
namespace chord { class LeaveRequest; }
namespace chord { class LeaveResponse; }
namespace chord { class NotifyRequest; }
namespace chord { class NotifyResponse; }
namespace chord { class StabilizeRequest; }
namespace chord { class StabilizeResponse; }
namespace chord { class SuccessorRequest; }
namespace chord { class SuccessorResponse; }
namespace chord { struct Context; }
namespace chord { struct Router; }
namespace chord { struct node; }
namespace spdlog { class logger; }

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

  grpc::Status leave(grpc::ServerContext *context, 
                     const chord::LeaveRequest *req,
                     chord::LeaveResponse *res) override;

  grpc::Status ping(grpc::ServerContext *context,
                    const chord::PingRequest *req,
                    chord::PingResponse *res) override;

  void fix_fingers(size_t index) override;

  signal<const node, const node>& on_predecessor_update() override {
    return event_predecessor_update;
  }

  ::grpc::Service* grpc_service() override {
    return this;
  }
 private:
  Context &context;
  Router *router;
  IClient *client;
  signal<const node, const node> event_predecessor_update;
  std::shared_ptr<spdlog::logger> logger;
};
}  // namespace chord
