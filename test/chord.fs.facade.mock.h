#pragma once

#include <iosfwd>
#include <grpcpp/impl/codegen/status.h>

#include "chord.fs.replication.h"
#include "chord.i.fs.facade.h"

namespace chord { class path; }
namespace chord { class uri; }

namespace chord {
namespace fs {

class MockFacade : public IFacade {
 public:
  virtual ~MockFacade() = default;

  MOCK_METHOD3(put, grpc::Status(const chord::path&, const chord::uri&, Replication repl));

  MOCK_METHOD2(get, grpc::Status(const chord::uri&, const chord::path&));

  MOCK_METHOD2(dir, grpc::Status(const chord::uri&, std::iostream&));

  MOCK_METHOD2(del, grpc::Status(const chord::uri&, const bool));
};

}  // namespace fs
}  // namespace chord
