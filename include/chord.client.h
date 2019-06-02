#pragma once

#include <functional>
#include <memory>

#include "chord.signal.h"
#include "chord.grpc.pb.h"
#include "chord.types.h"
#include "chord.uuid.h"
#include "chord.i.client.h"

namespace chord {
struct Router;
struct Context;
struct node;
}  // namespace chord

namespace spdlog {
class logger;
}  // namespace spdlog

namespace chord {

using StubFactory = std::function<std::unique_ptr<chord::Chord::StubInterface>(
    const endpoint_t &)>;

class Client : public IClient {
  static constexpr auto logger_name = "chord.client";

 private:
  const Context &context;
  Router *router;

  StubFactory make_stub;

  signal<void(const node)> event_predecessor_fail;
  signal<void(const node)> event_successor_fail;

  std::shared_ptr<spdlog::logger> logger;

  grpc::Status inform_predecessor_about_leave();
  grpc::Status inform_successour_about_leave();

 public:
  Client(const Context &context, Router *router);

  Client(const Context &context, Router *router, StubFactory factory);

  void leave() override;

  signal<void(const node)>& on_predecessor_fail() override;
  signal<void(const node)>& on_successor_fail() override;

  bool join(const endpoint_t &addr) override;
  grpc::Status join(const JoinRequest *req, JoinResponse *res) override;
  grpc::Status join(grpc::ClientContext *clientContext, const JoinRequest *req, JoinResponse *res) override;

  void stabilize() override;

  void notify() override;

  void check() override;

  chord::common::RouterEntry successor(const uuid_t &id) override;

  grpc::Status successor(grpc::ClientContext *context, const chord::SuccessorRequest *req, chord::SuccessorResponse *res) override;

  grpc::Status successor(const chord::SuccessorRequest *req, chord::SuccessorResponse *res) override;

};
} // namespace chord
