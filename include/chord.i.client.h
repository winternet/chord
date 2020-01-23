#pragma once

#include <functional>
#include <memory>

#include "chord.grpc.pb.h"
#include "chord.types.h"
#include "chord.uuid.h"
#include "chord.node.h"
#include "chord.signal.h"

namespace chord {

using StubFactory = std::function<std::unique_ptr<chord::Chord::StubInterface>(
    const endpoint &)>;

class IClient {

 public:

  virtual ~IClient() {}
  virtual void leave() =0;

  virtual grpc::Status ping(const node&) =0;
  virtual grpc::Status join(const endpoint& addr) =0;

  virtual void stabilize() =0;

  virtual grpc::Status notify() =0;
  virtual grpc::Status notify(const node&, const node&, const optional<node>&) =0;

  virtual void check() =0;

  virtual chord::common::RouterEntry lookup(const uuid_t &id) =0;

  virtual grpc::Status lookup(grpc::ClientContext *context, const chord::LookupRequest *req, chord::LookupResponse *res) =0;

  virtual grpc::Status lookup(const chord::LookupRequest *req, chord::LookupResponse *res) =0;

  virtual signal<void(const node)>& on_successor_fail() = 0;
  virtual signal<void(const node)>& on_predecessor_fail() = 0;

};

} // namespace chord
