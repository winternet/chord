#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <algorithm>
#include <iosfwd>
#include <set>
#include <string>
#include "chord.optional.h"
#include "chord.context.h"
#include "chord.exception.h"
#include "chord.file.h"
#include "chord.fs.metadata.h"
#include "chord.fs.metadata.manager.h"
#include "chord.fs.perms.h"
#include "chord.fs.replication.h"
#include "chord.fs.type.h"
#include "chord.path.h"
#include "chord.uri.h"

using namespace std;
using namespace chord;
using namespace chord::fs;
using ::testing::ElementsAre;

void cleanup(const Context& context) {
  std::string meta_dir = context.meta_directory;
  if(file::exists(meta_dir)) file::remove_all(meta_dir);
}

TEST(chord_metadata_manager, constructor_initializes_database) {
  Context context;
  cleanup(context);

  fs::MetadataManager metadata{context};
  ASSERT_TRUE(chord::file::is_directory(context.meta_directory));
  cleanup(context);
}

TEST(chord_metadata_manager, set_and_get) {
  Context context;
  cleanup(context);

  fs::MetadataManager metadata{context};
  auto uri = uri::from("chord:/folder/file1");
  fs::Metadata meta_set{"file1", "owner", "group", perms::all, type::regular};

  metadata.add(uri, {meta_set});
  set<fs::Metadata> meta_get = metadata.get(uri);

  fs::Metadata expected{"file1", "owner", "group", perms::all, type::regular};
  ASSERT_THAT(meta_get, ElementsAre(expected));
  cleanup(context);
}

TEST(chord_metadata_manager, set_and_get_updates_replication) {
  Context context;
  cleanup(context);

  fs::MetadataManager metadata{context};
  auto uri = uri::from("chord:/folder");
  fs::Metadata meta_set{"file1", "owner", "group", perms::all, type::regular, {}, {}, Replication(0,4)};
  fs::Metadata meta_root{".", "", "", perms::none, type::directory, {}, {}, Replication(0,4)};

  metadata.add(uri, {meta_root, meta_set});
  set<fs::Metadata> meta_get = metadata.get(uri);

  ASSERT_THAT(meta_get, ElementsAre(meta_root, meta_set));

  // update replication
  meta_set.replication = Replication(0,6);
  metadata.add(uri, {meta_set});
  meta_get = metadata.get(uri);

  //root's replication is adapted to maximum of all contents
  meta_root.replication = Replication{0,6};

  ASSERT_THAT(meta_get, ElementsAre(meta_root, meta_set));

  // check update of replication explicitly
  const auto repl = std::find_if(meta_get.begin(), meta_get.end(), [](const fs::Metadata& m) { return m.name == "."; })->replication;
  ASSERT_EQ(repl, Replication(0,6));

  cleanup(context);
}

TEST(chord_metadata_manager, set_delete_get) {
  Context context;
  cleanup(context);

  fs::MetadataManager metadata{context};
  auto uri = uri::from("chord:/folder");
  fs::Metadata meta_set;
  meta_set.name = "file1";

  metadata.add(uri, {meta_set});
  metadata.del(uri);
  try {
    metadata.get(uri);
    FAIL();
  } catch(const chord::exception &expected) {}
  cleanup(context);
}
