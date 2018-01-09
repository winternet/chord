#pragma once

#include <functional>
#include <memory>

#include "chord.facade.h"
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
 private:
  chord::Context* context;
  chord::ChordFacade* chord;

  StubFactory make_stub;

 public:
  Client(Context* context, chord::ChordFacade* chord);

  Client(Context* context, chord::ChordFacade* chord, StubFactory factory);

  grpc::Status put(const chord::uri &uri, std::istream &istream);

  grpc::Status get(const chord::uri &uri, std::ostream &ostream);

  grpc::Status meta(const chord::uri &notify_uri, const chord::path &file);
  //grpc::Status dir(const std::string& uri, std::ostream& ostream);
};

}  // namespace fs
}  // namespace chord
