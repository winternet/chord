#pragma once

#include <memory>
#include <functional>

#include "chord.log.h"
#include "chord.uuid.h"
#include "chord.peer.h"
#include "chord.router.h"
#include "chord.context.h"
#include "chord_fs.grpc.pb.h"

namespace chord {
  namespace fs {

    typedef std::function<std::unique_ptr<chord::fs::Filesystem::StubInterface>(const endpoint_t& endpoint)> StubFactory;
    class Client {
      private:
        const std::shared_ptr<Context>& context;
        const std::shared_ptr<chord::Peer>& peer;

        StubFactory make_stub;

      public:
        Client(const std::shared_ptr<Context>& context, const std::shared_ptr<chord::Peer>& peer);
        Client(const std::shared_ptr<Context>& context, const std::shared_ptr<chord::Peer>& peer, StubFactory factory);

        grpc::Status put(const std::string& uri, std::istream& istream);
        grpc::Status get(const std::string& uri, std::ostream& ostream);
    };

  } // namespace fs
} // namespace chord
