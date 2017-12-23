#pragma once

#include <memory>
#include <functional>

#include "chord.uuid.h"
#include "chord.grpc.pb.h"
#include "chord.types.h"

namespace chord {
struct Router;
struct Context;
}

namespace chord {

typedef std::function<std::unique_ptr<chord::Chord::StubInterface>(const endpoint_t &endpoint)> StubFactory;

class Client {
 private:
  Context &context;
  Router &router;

  StubFactory make_stub;

 public:
  Client(Context &context, Router &router);

  Client(Context &context, Router &router, StubFactory factory);

  void join(const endpoint_t &addr);

  void stabilize();

  void notify();

  void check();

  void fix_fingers();

  chord::common::RouterEntry successor(const uuid_t &id);

  grpc::Status
  successor(grpc::ClientContext *context, const chord::SuccessorRequest *req, chord::SuccessorResponse *res);

  grpc::Status successor(const chord::SuccessorRequest *req, chord::SuccessorResponse *res);

  grpc::Status put(const std::string &uri, std::istream &istream);

  grpc::Status get(const std::string &uri, std::ostream &ostream);
};
}
