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

namespace chord {
namespace fs {

using StubFactory = std::function<std::unique_ptr<chord::fs::Filesystem::StubInterface>(
    const endpoint_t &endpoint)>;

class Client {
 public:
  enum class Action { ADD, DEL, DIR };

 private:
  Context &context;
  chord::ChordFacade* chord;

  StubFactory make_stub;

void add_metadata(chord::fs::MetaRequest& req, const chord::path& path);

 public:
  Client(Context &context, chord::ChordFacade* chord);

  Client(Context &context, chord::ChordFacade* chord, StubFactory factory);

  grpc::Status put(const chord::uri &uri, std::istream &istream);

  grpc::Status get(const chord::uri &uri, std::ostream &ostream);

  grpc::Status del(const chord::uri &uri);

  grpc::Status dir(const chord::uri &uri, std::set<Metadata>& metadata);

  //TODO remove metadata for DEL / DIR
  grpc::Status meta(const chord::uri &uri, const Action &action);

  grpc::Status meta(const chord::uri &uri, const Action &action, std::set<Metadata>& metadata);

};

}  // namespace fs
}  // namespace chord
