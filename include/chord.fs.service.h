#pragma once

#include <grpc++/server_context.h>
#include <grpc/grpc.h>
#include <functional>

#include "chord.fs.client.h"
#include "chord.fs.metadata.manager.h"
#include "chord_fs.grpc.pb.h"

namespace chord {
struct Context;
}  // namespace chord
namespace spdlog {
class logger;
}  // namespace spdlog

namespace chord {
namespace fs {

using ClientFactory = std::function<chord::fs::Client()>;

class Service final : public chord::fs::Filesystem::Service {
  static constexpr auto logger_name = "chord.fs.service";

  grpc::Status get_from_reference(const chord::uri& uri);
 public:
  explicit Service(Context &context, ChordFacade* chord);

  grpc::Status put(grpc::ServerContext *context,
                   grpc::ServerReader<chord::fs::PutRequest> *reader,
                   chord::fs::PutResponse *response) override;

  grpc::Status get(grpc::ServerContext *context,
                   const chord::fs::GetRequest *req,
                   grpc::ServerWriter<chord::fs::GetResponse> *writer) override;

  grpc::Status del(grpc::ServerContext *context,
                   const chord::fs::DelRequest *request,
                   chord::fs::DelResponse *response) override;

  grpc::Status meta(grpc::ServerContext *serverContext,
                    const chord::fs::MetaRequest *request,
                    chord::fs::MetaResponse *response) override;

  MetadataManager* metadata_manager() {
    return metadata_mgr.get();
  }

 private:
  Context &context;
  ChordFacade *chord;
  std::unique_ptr<MetadataManager> metadata_mgr;
  ClientFactory make_client;
  std::shared_ptr<spdlog::logger> logger;
};

} //namespace fs
} //namespace chord
