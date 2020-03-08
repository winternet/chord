#include "chord.fs.context.metadata.h"

#include <grpcpp/impl/codegen/client_context.h>
#include <grpcpp/impl/codegen/server_context.h>
#include <string>
#include <cstdlib>

#include "chord.fs.metadata.h"
#include "chord.fs.replication.h"
#include "chord.uri.h"
#include "chord.uuid.h"
#include "chord.optional.h"
#include "chord.exception.h"

using grpc::ClientContext;
using grpc::ServerContext;

namespace chord {
namespace fs {

client::options ContextMetadata::from(const grpc::ServerContext* serverContext) {
  client::options options;
  options.replication = replication_from(serverContext);
  options.source = src_from(serverContext);
  options.rebalance = rebalance_from(serverContext);
  return options;
}

void ContextMetadata::add(ClientContext& context, const Metadata& metadata) {
  if(metadata.file_hash) {
    context.AddMetadata(ContextMetadata::file_hash, metadata.file_hash.value().string());
  }
  add(context, metadata.replication);
}
void ContextMetadata::add(grpc::ClientContext& context, const chord::uri& uri) {
  context.AddMetadata(ContextMetadata::uri, to_string(uri));
}
void ContextMetadata::add(grpc::ClientContext& context, const chord::optional<chord::uuid>& hash) {
  if(hash) context.AddMetadata(ContextMetadata::file_hash, *hash);
}

void ContextMetadata::add_src(grpc::ClientContext& context, const chord::uuid& src) {
  context.AddMetadata(ContextMetadata::src, src);
}

void ContextMetadata::add_rebalance(grpc::ClientContext& context, const bool rebalance) {
  context.AddMetadata(ContextMetadata::rebalance, grpc::to_string(rebalance));
}

void ContextMetadata::set_file_hash_equal(grpc::ServerContext* context, const bool metadata_only) {
  context->AddInitialMetadata(ContextMetadata::file_hash_equal, metadata_only ? "true" : "false");
}

void ContextMetadata::add(ClientContext& context, const Replication& repl) {
  context.AddMetadata(ContextMetadata::replication_index, std::to_string(repl.index));
  context.AddMetadata(ContextMetadata::replication_count, std::to_string(repl.count));
}

chord::optional<chord::uuid> ContextMetadata::file_hash_from(const grpc::ServerContext* serverContext) {
  const auto metadata = serverContext->client_metadata();
  if(metadata.count(ContextMetadata::file_hash) > 0) {
    const auto file_hash = metadata.find(ContextMetadata::file_hash)->second;
    return chord::uuid{std::string(file_hash.begin(), file_hash.end())};
  }
  return {};
}

chord::uri ContextMetadata::uri_from(const grpc::ServerContext* serverContext) {
  const auto metadata = serverContext->client_metadata();
  if(metadata.count(ContextMetadata::uri) == 0) {
    throw__exception("missing uri metadata in server context.");
  }
  const auto uri_gstr = metadata.find(ContextMetadata::uri)->second;
  return {std::string(uri_gstr.begin(), uri_gstr.end())};
}

Replication ContextMetadata::replication_from(const ServerContext* serverContext) {
  const auto metadata = serverContext->client_metadata();
  Replication repl;
  if(metadata.count(ContextMetadata::replication_index) > 0 
      && metadata.count(ContextMetadata::replication_count) > 0) {
    const auto idx = metadata.find(ContextMetadata::replication_index)->second;
    const auto cnt = metadata.find(ContextMetadata::replication_count)->second;

    repl.index = static_cast<decltype(repl.index)>(std::strtoul(idx.data(), nullptr, 0));
    repl.count = static_cast<decltype(repl.count)>(std::strtoul(cnt.data(), nullptr, 0));
  }
  return repl;
}

chord::optional<chord::uuid> ContextMetadata::src_from(const grpc::ServerContext* serverContext) {
  const auto metadata = serverContext->client_metadata();
  if(metadata.count(ContextMetadata::src) == 0) {
    return {};
  }
  const auto id_gstr = metadata.find(ContextMetadata::src)->second;
  return chord::uuid{std::string(id_gstr.begin(), id_gstr.end())};
}

bool ContextMetadata::file_hash_equal_from(const grpc::ClientContext& clientContext) {
  const auto metadata = clientContext.GetServerInitialMetadata();
  if(metadata.count(ContextMetadata::file_hash_equal) > 0) {
    const auto& val = metadata.find(ContextMetadata::file_hash_equal)->second;
    return (val == "1" || val == "true");
  }
  return false;
}

bool ContextMetadata::rebalance_from(const grpc::ServerContext* serverContext) {
  const auto metadata = serverContext->client_metadata();
  if(metadata.count(ContextMetadata::rebalance) > 0) {
    const auto& val = metadata.find(ContextMetadata::rebalance)->second;
    return (val == "1" || val == "true");
  }
  return false;
}

}
}
