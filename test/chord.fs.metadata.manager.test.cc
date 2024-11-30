#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "chord.fs.metadata.manager.h"

using namespace std;
using namespace chord;
using namespace chord::fs;

using testing::ElementsAre;

TEST(MetadataManager, sorted_returns) {
  IMetadataManager::uri_meta_map_desc map;
  const auto root = chord::uri{"chord:///root.md"};
  const auto aa = chord::uri{"chord:///aa/aa.md"};
  const auto bb = chord::uri{"chord:///aa/bb.md"};
  const auto cc = chord::uri{"chord:///aa/bb/cc.md"};
  const auto ee = chord::uri{"chord:///cc/dd/ee.md"};
  map[root];
  map[aa];
  map[bb];
  map[cc];
  map[ee];

  std::vector<chord::uri> keys;
  for(const auto& [k,_]:map) { keys.push_back(k); }

  ASSERT_THAT(keys, ElementsAre(ee, cc, bb, aa, root));
}

