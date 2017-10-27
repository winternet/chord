#pragma once
#include <functional>

#include <grpc/grpc.h>

#include <grpc++/server.h>
#include <grpc++/server_builder.h>
#include <grpc++/server_context.h>
#include <grpc++/security/server_credentials.h>

#include "chord.grpc.pb.h"

#include "chord.peer.h"


class ChordControlService final : public chord::ChordControl::Service {

public:
  ChordControlService(const std::shared_ptr<ChordPeer>& peer);

  Status control(grpc::ServerContext* context, const chord::ControlRequest* req, chord::ControlResponse* res) override;

private:
  Status parse_command(const chord::ControlRequest* req, chord::ControlResponse* res);

  const std::shared_ptr<ChordPeer>& peer;
};
