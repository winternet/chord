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

  MOCK_METHOD3(put, void(const chord::path&, const chord::uri&, Replication repl));

  MOCK_METHOD2(get, void(const chord::uri&, const chord::path&));

  MOCK_METHOD2(dir, void(const chord::uri&, std::iostream&));

  MOCK_METHOD1(del, void(const chord::uri&));
};

}  // namespace fs
}  // namespace chord
