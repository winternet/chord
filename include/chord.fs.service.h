#pragma once

#include <grpc++/server_context.h>
#include <grpc/grpc.h>
#include <functional>

#include "chord.uuid.h"
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
  grpc::Status del(const chord::uri& uri);
 public:
  explicit Service(Context &context, ChordFacade* chord);
  explicit Service(Context &context, ChordFacade* chord, IMetadataManager* metadata_mgr);

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

  grpc::Status take(grpc::ServerContext *context, 
                    const chord::fs::TakeRequest *req,
                    grpc::ServerWriter<chord::fs::TakeResponse> *writer) override;

  IMetadataManager* metadata_manager() {
    return metadata_mgr.get();
  }

  //TODO move to cc
  void set_take_callback(const take_producer_t callback) {
    take_producer_callback = callback;
  }

  //TODO move to cc
  void set_on_leave_callback(const take_consumer_t callback) {
    on_leave_callback = callback;
  }

  std::vector<chord::fs::TakeResponse> produce_take_response(const chord::uuid& from, const chord::uuid& to) {
      const auto map = metadata_mgr->get(from, to);
      std::vector<chord::fs::TakeResponse> ret;
      for (const auto& m : map) {
        chord::fs::TakeResponse res;
        chord::fs::MetaResponse meta;

        meta.set_uri(m.first);

        auto* node_ref = meta.mutable_node_ref();
        node_ref->set_uuid(context.uuid());
        node_ref->set_endpoint(context.bind_addr);

        MetadataBuilder::addMetadata(m.second, meta);
        res.set_id(m.first);
        res.mutable_meta()->CopyFrom(meta);
        //res.mutable_detail()->PackFrom(meta);
        ret.push_back(res);
      }
      return ret;
  }
 private:
  Context &context;
  ChordFacade *chord;
  std::unique_ptr<IMetadataManager> metadata_mgr;
  ClientFactory make_client;
  std::shared_ptr<spdlog::logger> logger;
  // callbacks
  take_producer_t take_producer_callback;
  take_consumer_t on_leave_callback;
};

} //namespace fs
} //namespace chord
