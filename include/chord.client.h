#pragma once

#include <functional>
#include <memory>
#include <grpcpp/client_context.h>

#include "chord_common.pb.h"
#include "chord.signal.h"
#include "chord.grpc.pb.h"
#include "chord.types.h"
#include "chord.uuid.h"
#include "chord.i.client.h"

namespace chord { class JoinRequest; }                                                                                                         
namespace chord { class JoinResponse; }                                                                                                        
namespace chord { class SuccessorRequest; }                                                                                                    
namespace chord { class SuccessorResponse; }
namespace chord { struct Context; }
namespace chord { struct Router; }
namespace chord { struct node; }
namespace spdlog { class logger; }

namespace chord {

using StubFactory = std::function<std::unique_ptr<chord::Chord::StubInterface>(
    const endpoint &)>;

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

  grpc::Status join(const endpoint& addr) override;
  grpc::Status join(const JoinRequest *req, JoinResponse *res) override;
  grpc::Status join(grpc::ClientContext *clientContext, const JoinRequest *req, JoinResponse *res) override;

  void stabilize() override;

  grpc::Status notify() override;
  grpc::Status notify(const node&, const node&, const node&) override;

  void check() override;

  chord::common::RouterEntry successor(const uuid_t &id) override;

  grpc::Status successor(grpc::ClientContext *context, const chord::SuccessorRequest *req, chord::SuccessorResponse *res) override;

  grpc::Status successor(const chord::SuccessorRequest *req, chord::SuccessorResponse *res) override;

};
} // namespace chord
