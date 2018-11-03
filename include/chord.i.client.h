#pragma once

#include <functional>
#include <memory>

#include "chord.grpc.pb.h"
#include "chord.types.h"
#include "chord.uuid.h"
#include "chord.node.h"
#include "chord.i.callback.h"

namespace chord {

using StubFactory = std::function<std::unique_ptr<chord::Chord::StubInterface>(
    const endpoint_t &)>;

class IClient {

 public:

  virtual ~IClient() {}
  virtual void leave() =0;

  virtual bool join(const endpoint_t &addr) =0;
  virtual grpc::Status join(const JoinRequest *req, JoinResponse *res) =0;
  virtual grpc::Status join(grpc::ClientContext *clientContext, const JoinRequest *req, JoinResponse *res) =0;

  virtual void take() =0;
  virtual void take(const uuid from, const uuid to, const node responsible, const take_consumer_t callback) =0;

  virtual void stabilize() =0;

  virtual void notify() =0;

  virtual void check() =0;

  virtual chord::common::RouterEntry successor(const uuid_t &id) =0;

  virtual grpc::Status successor(grpc::ClientContext *context, const chord::SuccessorRequest *req, chord::SuccessorResponse *res) =0;

  virtual grpc::Status successor(const chord::SuccessorRequest *req, chord::SuccessorResponse *res) =0;

};
} // namespace chord