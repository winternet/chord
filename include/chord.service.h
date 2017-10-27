#pragma once
#include <functional>

#include <grpc/grpc.h>

#include <grpc++/server.h>
#include <grpc++/server_builder.h>
#include <grpc++/server_context.h>
#include <grpc++/security/server_credentials.h>

#include "chord.context.h"
#include "chord.grpc.pb.h"
#include "chord.i.service.h"
#include "chord.exception.h"

#include "chord.client.h"


typedef std::function<ChordClient()> ClientFactory;

class ChordService final : public chord::Chord::Service, AbstractService {

protected:

  chord::Header make_header();

public:
  ChordService(Context& context, Router& router);

  ChordService(Context& context, Router& router, ClientFactory make_client);

  Status join(ServerContext* context, const chord::JoinRequest* req, chord::JoinResponse* res) override;

  virtual RouterEntry successor(const uuid_t& uuid) noexcept(false) override ;
  virtual Status successor(ServerContext* context, const chord::SuccessorRequest* req, chord::SuccessorResponse* res) override;

  virtual Status stabilize(grpc::ServerContext* context, const chord::StabilizeRequest* req, chord::StabilizeResponse* res) override;

  virtual Status notify(grpc::ServerContext* context, const chord::NotifyRequest* req, chord::NotifyResponse* res) override;

  virtual Status check(grpc::ServerContext* context, const chord::CheckRequest* req, chord::CheckResponse* res) override;

  virtual Status put(grpc::ServerContext* context, grpc::ServerReader<chord::PutRequest>* reader, chord::PutResponse* response) override;

  void fix_fingers(size_t index);

private:
  Context& context;
  Router& router;
  ClientFactory make_client;
};
