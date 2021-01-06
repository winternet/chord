#pragma once

#include <iosfwd>
#include <grpcpp/impl/codegen/status.h>

#include "chord.fs.replication.h"
#include "chord.i.fs.facade.h"
#include "chord.fs.metadata.h"

namespace chord { class path; }
namespace chord { class uri; }

namespace chord {
namespace fs {

class MockFacade : public IFacade {
 public:
  virtual ~MockFacade() = default;

  MOCK_METHOD2(get, grpc::Status(const chord::uri&, const chord::path&));
  MOCK_METHOD3(put, grpc::Status(const chord::path&, const chord::uri&, Replication));
  MOCK_METHOD2(del, grpc::Status(const chord::uri&, const bool));
  MOCK_METHOD2(mkdir, grpc::Status(const chord::uri&, Replication));
  MOCK_METHOD3(move, grpc::Status(const chord::uri&, const chord::uri&, const bool));

  MOCK_METHOD2(dir, grpc::Status(const chord::uri&, std::set<Metadata>&));
  MOCK_METHOD2(dir, grpc::Status(const chord::uri&, std::ostream&));
  MOCK_METHOD1(exists, grpc::Status(const chord::uri&));

};

}  // namespace fs
}  // namespace chord
