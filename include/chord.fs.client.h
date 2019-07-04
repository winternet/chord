#pragma once

#include <grpcpp/impl/codegen/status.h>
#include <functional>
#include <iosfwd>
#include <memory>
#include <set>

#include "chord.fs.replication.h"
#include "chord.types.h"
#include "chord.uri.h"
#include "chord_fs.grpc.pb.h"
#include "chord.i.fs.metadata.manager.h"

namespace chord { class path; }
namespace chord { class ChordFacade; }
namespace chord { namespace fs { class DelRequest; } }
namespace chord { namespace fs { class MetaRequest; } }
namespace chord { namespace fs { struct Metadata; } }
namespace chord { struct Context; }
namespace chord { struct node; }
namespace spdlog { class logger; }

namespace spdlog {
  class logger;
}  // namespace spdlog
namespace chord {
namespace fs {

using StubFactory = std::function<std::unique_ptr<chord::fs::Filesystem::StubInterface>(
    const endpoint& endpoint)>;

class Client {
 public:
  enum class Action { ADD, DEL, DIR };

 private:
  static constexpr auto logger_name = "chord.fs.client";

  Context &context;
  chord::ChordFacade* chord;
  chord::fs::IMetadataManager* metadata_mgr;

  StubFactory make_stub;
  std::shared_ptr<spdlog::logger> logger;

 public:
  Client(Context &context, chord::ChordFacade* chord, chord::fs::IMetadataManager* metadata_mgr);

  Client(Context &context, chord::ChordFacade* chord, StubFactory factory);

  // called internally by the chord.fs.service
  grpc::Status put(const chord::node&, const chord::uri&, std::istream&, Replication repl);
  grpc::Status put(const chord::node&, const chord::uri&, const chord::path&, Replication repl);

  grpc::Status put(const chord::uri&, std::istream&, Replication repl = Replication() );
  grpc::Status put(const chord::uri&, const chord::path&, Replication repl);

  grpc::Status get(const chord::uri&, std::ostream&);
  grpc::Status get(const chord::uri&, const chord::path&);
  grpc::Status get(const chord::uri&, const chord::node&, std::ostream&);
  grpc::Status get(const chord::uri&, const chord::node&, const chord::path&);

  grpc::Status del(const chord::uri&, const bool recursive=false);
  grpc::Status del(const chord::node&, const chord::uri&, const bool recursive=false);
  grpc::Status del(const chord::node&, const DelRequest*);

  grpc::Status dir(const chord::uri&, std::set<Metadata>&);

  //TODO remove metadata for DEL / DIR
  grpc::Status meta(const chord::uri&, const Action&, std::set<Metadata>&);
  grpc::Status meta(const chord::uri&, const Action&);

  // called internally by the chord.fs.service
  grpc::Status meta(const chord::node&, const chord::uri&, const Action&, std::set<Metadata>&);

};

}  // namespace fs
}  // namespace chord
