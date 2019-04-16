#pragma once

#include <functional>
#include <memory>
#include <set>

#include "chord.facade.h"
#include "chord.fs.metadata.h"
#include "chord.fs.replication.h"
#include "chord.types.h"
#include "chord.uri.h"
#include "chord_fs.grpc.pb.h"

namespace chord {
class Peer;

struct Context;
struct Router;
}  // namespace chord

namespace spdlog {
  class logger;
}  // namespace spdlog
namespace chord {
namespace fs {

using StubFactory = std::function<std::unique_ptr<chord::fs::Filesystem::StubInterface>(
    const endpoint_t &endpoint)>;

class Client {
 public:
  enum class Action { ADD, DEL, DIR };

 private:
  static constexpr auto logger_name = "chord.fs.client";

  Context &context;
  chord::ChordFacade* chord;

  StubFactory make_stub;
  std::shared_ptr<spdlog::logger> logger;

void add_metadata(chord::fs::MetaRequest& req, const chord::path& path);

 public:
  Client(Context &context, chord::ChordFacade* chord);

  Client(Context &context, chord::ChordFacade* chord, StubFactory factory);

  // called internally by the chord.fs.service
  grpc::Status put(const chord::node&, const chord::uuid&, const chord::uri&, std::istream&, Replication repl);

  grpc::Status put(const chord::uri&, std::istream&, Replication repl = Replication() );

  grpc::Status get(const chord::uri&, std::ostream&);

  grpc::Status get(const chord::uri&, const chord::node&, std::ostream&);

  grpc::Status del(const chord::uri&, const bool recursive=false, const Replication repl = Replication());
  grpc::Status del(const chord::node&, const chord::uri&, const bool recursive=false, const Replication repl = Replication());
  grpc::Status del(const chord::node&, const DelRequest*);

  grpc::Status dir(const chord::uri&, std::set<Metadata>&);

  //void take() ;
  void take(const uuid from, const uuid to, const chord::node responsible, const take_consumer_t callback);

  //TODO remove metadata for DEL / DIR
  grpc::Status meta(const chord::uri&, const Action&, std::set<Metadata>&);
  grpc::Status meta(const chord::uri&, const Action&);

  // called internally by the chord.fs.service
  grpc::Status meta(const chord::node&, const chord::uri&, const Action&, std::set<Metadata>&);

};

}  // namespace fs
}  // namespace chord
