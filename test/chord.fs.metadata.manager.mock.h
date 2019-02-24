#pragma once

#include <set>
#include <map>

#include "chord.fs.metadata.h"
#include "chord.uri.h"

namespace chord {
namespace fs {

class MockMetadataManager : public IMetadataManager {

 public:
  virtual ~MockMetadataManager() = default;

  MOCK_METHOD1(del, std::set<Metadata>(const chord::uri&));

  MOCK_METHOD2(del, std::set<Metadata>(const chord::uri&, const std::set<Metadata> &));

  MOCK_METHOD1(dir, std::set<Metadata>(const chord::uri&));

  MOCK_METHOD2(add, bool(const chord::uri&, const std::set<Metadata>&));

  MOCK_METHOD2(get, std::map<chord::uri, std::set<Metadata> >(const chord::uuid&, const chord::uuid&));

  MOCK_METHOD1(get, std::map<chord::uri, std::set<Metadata>>(const chord::node&));
  MOCK_METHOD1(get, std::set<Metadata>(const chord::uri&));

  MOCK_METHOD1(exists, bool(const chord::uri&));
};

} //namespace fs
} //namespace chord