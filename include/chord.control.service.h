#pragma once
#include <functional>

#include <grpc/grpc.h>

#include <grpc++/server.h>
#include <grpc++/server_builder.h>
#include <grpc++/server_context.h>
#include <grpc++/security/server_credentials.h>

#include "chord.grpc.pb.h"

#include "chord.peer.h"

using grpc::ServerContext;
using grpc::ServerBuilder;
using grpc::Status;

using chord::ControlResponse;
using chord::ControlRequest;

class ChordControlService final : public chord::ChordControl::Service {

public:
  ChordControlService(const std::shared_ptr<ChordPeer>& peer);

  Status control(ServerContext* context, const ControlRequest* req, ControlResponse* res);

  void parse_command(const std::string& command);

private:
  const std::shared_ptr<ChordPeer>& peer;
};
