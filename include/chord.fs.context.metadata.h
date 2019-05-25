#pragma once

namespace grpc {
  struct ClientContext;
  struct ServerContext;
}

namespace chord {
namespace fs {

struct Replication;

struct ContextMetadata {
  static constexpr auto replication_count = "replication.count";
  static constexpr auto replication_index = "replication.index";

  static void add(grpc::ClientContext& context, const chord::fs::Replication& repl);
  static Replication replication_from(const grpc::ServerContext* context);
};

}
}
