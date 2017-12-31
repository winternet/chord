#pragma once

#include <functional>

#include <grpc/grpc.h>
#include <grpc++/server_context.h>

#include "chord.metadata.manager.h"
#include "chord_fs.grpc.pb.h"

namespace chord {
struct Context;
}

namespace chord {
namespace fs {

class Service final : public chord::fs::Filesystem::Service {

 private:
	Metadata convert(const NotifyRequest *request);

 public:
  explicit Service(Context *context);

  grpc::Status put(grpc::ServerContext *context, grpc::ServerReader<chord::fs::PutRequest> *reader,
                           chord::fs::PutResponse *response) override;

  grpc::Status get(grpc::ServerContext *context, const chord::fs::GetRequest *req,
                           grpc::ServerWriter<chord::fs::GetResponse> *writer) override;

  grpc::Status notify(grpc::ServerContext *serverContext, const chord::fs::NotifyRequest *request,
                              chord::fs::NotifyResponse *response) override;

 private:
  Context *context;
  std::unique_ptr<MetadataManager> metadata;
};

} //namespace fs
} //namespace chord
