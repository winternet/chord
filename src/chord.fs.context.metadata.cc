#include "chord.fs.context.metadata.h"

#include <grpcpp/impl/codegen/client_context.h>
#include <grpcpp/impl/codegen/server_context.h>
#include <string>
#include <cstdlib>

#include "chord.fs.replication.h"

using grpc::ClientContext;
using grpc::ServerContext;

namespace chord {
namespace fs {

void ContextMetadata::add(ClientContext& context, const Replication& repl) {
  context.AddMetadata(ContextMetadata::replication_index, std::to_string(repl.index));
  context.AddMetadata(ContextMetadata::replication_count, std::to_string(repl.count));
}

Replication ContextMetadata::replication_from(const ServerContext* context) {
  const auto metadata = context->client_metadata();
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

}
}
