#pragma once

#include <functional>
#include <memory>

#include "chord.grpc.pb.h"
#include "chord.types.h"
#include "chord.uuid.h"
#include "chord.node.h"
#include "chord.signal.h"

namespace chord {

using StubFactory = std::function<std::unique_ptr<chord::Chord::StubInterface>(const node &)>;

class IClient {

 public:

  virtual ~IClient() = default;
  virtual void leave() =0;

  virtual grpc::Status join(const endpoint& addr) =0;
  virtual grpc::Status join(const JoinRequest *req, JoinResponse *res) =0;
  virtual grpc::Status join(grpc::ClientContext *clientContext, const JoinRequest *req, JoinResponse *res) =0;

  virtual grpc::Status ping(const node& addr) =0;

  virtual void stabilize() =0;

  virtual grpc::Status notify() =0;
  virtual grpc::Status notify(const node&, const node&, const node&) =0;

  virtual void check() =0;

  virtual chord::common::RouterEntry successor(const uuid_t &id) =0;

  virtual grpc::Status successor(grpc::ClientContext *context, const chord::SuccessorRequest *req, chord::SuccessorResponse *res) =0;

  virtual grpc::Status successor(const chord::SuccessorRequest *req, chord::SuccessorResponse *res) =0;

  virtual signal<const node>& on_successor_fail() = 0;
  virtual signal<const node>& on_predecessor_fail() = 0;

};

} // namespace chord
