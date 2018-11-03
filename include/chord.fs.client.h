#pragma once

#include <functional>
#include <memory>
#include <set>

#include "chord.facade.h"
#include "chord.fs.metadata.h"
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
  grpc::Status put(const chord::node& target, const chord::uuid& hash, const chord::uri &uri, std::istream &istream, const size_t repl_cnt);

  grpc::Status put(const chord::uri &uri, std::istream &istream, const size_t repl_cnt = 1);

  grpc::Status get(const chord::uri &uri, std::ostream &ostream);

  grpc::Status get(const chord::uri &uri, const chord::node& node, std::ostream &ostream);

  grpc::Status del(const chord::uri &uri);

  grpc::Status del(const chord::uri &uri, const chord::node& node);

  grpc::Status dir(const chord::uri &uri, std::set<Metadata>& metadata);

  //TODO remove metadata for DEL / DIR
  grpc::Status meta(const chord::uri &uri, const Action &action, std::set<Metadata>& metadata, const size_t repl_cnt = 1);
  grpc::Status meta(const chord::uri &uri, const Action &action, const size_t repl_cnt = 1);

  // called internally by the chord.fs.service
  grpc::Status meta(const chord::node& target, const chord::uri &uri, const Action &action, std::set<Metadata>& metadata, const size_t repl_cnt = 1);

};

}  // namespace fs
}  // namespace chord
