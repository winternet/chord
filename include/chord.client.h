#pragma once

#include <functional>
#include <memory>

#include "chord.grpc.pb.h"
#include "chord.types.h"
#include "chord.uuid.h"

namespace chord {
struct Router;
struct Context;
}  // namespace chord

namespace chord {

using StubFactory = std::function<std::unique_ptr<chord::Chord::StubInterface>(
    const endpoint_t &)>;

class Client {
 private:
  Context &context;
  Router *router;

  StubFactory make_stub;

 public:
  Client(Context &context, Router *router);

  Client(Context &context, Router *router, StubFactory factory);

  void join(const endpoint_t &addr);

  void stabilize();

  void notify();

  void check();

  chord::common::RouterEntry successor(const uuid_t &id);

  grpc::Status
  successor(grpc::ClientContext *context, const chord::SuccessorRequest *req, chord::SuccessorResponse *res);

  grpc::Status successor(const chord::SuccessorRequest *req, chord::SuccessorResponse *res);

};
} // namespace chord
