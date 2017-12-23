#pragma once

#include <functional>

#include <grpc/grpc.h>
#include <grpc++/server_context.h>

#include "chord_fs.grpc.pb.h"

namespace chord {
struct Context;
}

namespace chord {
namespace fs {

class Service final : public chord::fs::Filesystem::Service {

 public:
  Service(Context &context);

  virtual grpc::Status put(grpc::ServerContext *context, grpc::ServerReader<chord::fs::PutRequest> *reader,
                           chord::fs::PutResponse *response) override;

  virtual grpc::Status get(grpc::ServerContext *context, const chord::fs::GetRequest *req,
                           grpc::ServerWriter<chord::fs::GetResponse> *writer) override;

  virtual grpc::Status notify(grpc::ServerContext *serverContext, const chord::fs::NotifyRequest *request,
                              chord::fs::NotifyResponse *response) override;

 private:
  Context &context;
};

} //namespace fs
} //namespace chord
