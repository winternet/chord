#pragma once

#include <functional>

#include <grpc/grpc.h>
#include <grpc++/server_context.h>

#include "chord.pb.h"
#include "chord.grpc.pb.h"
#include "chord.i.service.h"
#include "chord.exception.h"

#include "chord.client.h"

namespace chord {
struct Context;
struct Router;
}

namespace chord {
typedef std::function<chord::Client()> ClientFactory;

class Service final : public chord::Chord::Service, AbstractService {

 public:
  Service(Context &context, Router &router);

  Service(Context &context, Router &router, ClientFactory make_client);

  grpc::Status join(grpc::ServerContext *context, const chord::JoinRequest *req, chord::JoinResponse *res) override;

  virtual chord::common::RouterEntry successor(const uuid_t &uuid) noexcept(false) override;

  virtual grpc::Status
  successor(grpc::ServerContext *context, const chord::SuccessorRequest *req, chord::SuccessorResponse *res) override;

  virtual grpc::Status
  stabilize(grpc::ServerContext *context, const chord::StabilizeRequest *req, chord::StabilizeResponse *res) override;

  virtual grpc::Status
  notify(grpc::ServerContext *context, const chord::NotifyRequest *req, chord::NotifyResponse *res) override;

  virtual grpc::Status
  check(grpc::ServerContext *context, const chord::CheckRequest *req, chord::CheckResponse *res) override;

  void fix_fingers(size_t index);

 private:
  Context &context;
  Router &router;
  ClientFactory make_client;
};
}
