#pragma once
#include <grpcpp/server_context.h>
#include <grpcpp/client_context.h>

#include "chord.uuid.h"
#include "chord.uri.h"
#include "chord.optional.h"
#include "chord.fs.replication.h"
#include "chord.fs.client.options.h"

namespace chord { namespace fs { class Metadata; } }

namespace chord {
namespace fs {

struct Replication;

struct ContextMetadata {
  static constexpr auto replication_count = "replication.count";
  static constexpr auto replication_index = "replication.index";
  static constexpr auto file_hash = "file.hash";
  static constexpr auto uri = "uri";
  static constexpr auto src = "src";
  static constexpr auto file_hash_equal = "file.hash.equal";
  static constexpr auto rebalance = "rebalance";

  static client::options from(const grpc::ServerContext*);

  static void add(grpc::ClientContext&, const chord::fs::Metadata&);
  static void add(grpc::ClientContext&, const chord::fs::Replication&);
  static void add(grpc::ClientContext&, const chord::uri&);
  static void add(grpc::ClientContext&, std::istream&);
  static void add(grpc::ClientContext&, const chord::optional<chord::uuid>&);
  static void add_src(grpc::ClientContext& context);
  static void add_src(grpc::ClientContext&, const chord::uuid&);
  static void set_file_hash_equal(grpc::ServerContext* context, const bool=true);

  static void add_rebalance(grpc::ClientContext&, const bool);

  static chord::fs::Replication replication_from(const grpc::ServerContext*);
  static chord::optional<chord::uuid> file_hash_from(const grpc::ServerContext*);
  static chord::uri uri_from(const grpc::ServerContext*);
  static chord::optional<chord::uuid> src_from(const grpc::ServerContext*);
  static bool file_hash_equal_from(const grpc::ClientContext&);
  static bool rebalance_from(const grpc::ServerContext*);
};

}
}
