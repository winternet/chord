#pragma once

#include <set>

#include "chord.fs.client.h"
#include "chord.fs.service.h"
#include "chord.log.h"
#include "chord.uri.h"

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
