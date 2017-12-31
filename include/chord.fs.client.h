#pragma once

#include <memory>
#include <functional>

#include "chord.types.h"
#include "chord_fs.grpc.pb.h"
#include "chord.facade.h"

namespace chord {
class Peer;

struct Context;
struct Router;
}

namespace chord {
namespace fs {

typedef std::function<std::unique_ptr<chord::fs::Filesystem::StubInterface>(
    const endpoint_t &endpoint)> StubFactory;

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

  grpc::Status notify(const chord::uri &notify_uri, const chord::path &file);
  //grpc::Status dir(const std::string& uri, std::ostream& ostream);
};

} // namespace fs
} // namespace chord
