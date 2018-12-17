#pragma once

#include <functional>
#include <memory>

#include "chord.grpc.pb.h"
#include "chord.types.h"
#include "chord.uuid.h"
#include "chord.i.callback.h"
#include "chord.i.client.h"

namespace chord {
struct Router;
struct Context;
class node;
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
  take_consumer_t take_consumer_callback;
  std::shared_ptr<spdlog::logger> logger;

 public:
  Client(const Context &context, Router *router);

  Client(const Context &context, Router *router, StubFactory factory);

  void leave() override;

  bool join(const endpoint_t &addr) override;
  grpc::Status join(const JoinRequest *req, JoinResponse *res) override;
  grpc::Status join(grpc::ClientContext *clientContext, const JoinRequest *req, JoinResponse *res) override;

  void stabilize() override;

  void notify() override;

  void check() override;

  //TODO move to cc
  void set_take_callback(const take_consumer_t callback) {
    take_consumer_callback = callback;
  }

  chord::common::RouterEntry successor(const uuid_t &id) override;

  grpc::Status successor(grpc::ClientContext *context, const chord::SuccessorRequest *req, chord::SuccessorResponse *res) override;

  grpc::Status successor(const chord::SuccessorRequest *req, chord::SuccessorResponse *res) override;

};
} // namespace chord
