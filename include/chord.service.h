#pragma once
#include <functional>

#include <grpc/grpc.h>

#include <grpc++/server.h>
#include <grpc++/server_builder.h>
#include <grpc++/server_context.h>
#include <grpc++/security/server_credentials.h>

#include "chord.context.h"
#include "chord.grpc.pb.h"

#include "chord.client.h"

using grpc::ServerContext;
using grpc::ServerBuilder;
using grpc::Status;

using chord::JoinResponse;
using chord::JoinRequest;
using chord::SuccessorResponse;
using chord::SuccessorRequest;
using chord::StabilizeResponse;
using chord::StabilizeRequest;
using chord::NotifyResponse;
using chord::NotifyRequest;
using chord::RouterEntry;
using chord::Chord;

typedef std::function<ChordClient()> ClientFactory;

class ChordServiceImpl final : public Chord::Service {

protected:

  Header make_header();

public:
  ChordServiceImpl(std::shared_ptr<Context> context);

  ChordServiceImpl(std::shared_ptr<Context> context, ClientFactory make_client);

  Status join(ServerContext* context, const JoinRequest* req, JoinResponse* res);

  void successor(const uuid_t& uuid);
  Status successor(ServerContext* context, const SuccessorRequest* req, SuccessorResponse* res);

  Status stabilize(ServerContext* context, const StabilizeRequest* req, StabilizeResponse* res);

  Status notify(ServerContext* context, const NotifyRequest* req, NotifyResponse* res);

  Status check(ServerContext* context, const CheckRequest* req, CheckResponse* res);

  void fix_fingers(size_t index);

private:
  std::shared_ptr<Context> context { nullptr };
  std::shared_ptr<Router> router { nullptr };
  ClientFactory make_client;
};
