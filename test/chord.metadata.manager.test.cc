#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "chord.file.h"
#include "chord.metadata.manager.h"

using namespace std;
using namespace chord;
using namespace chord::fs;
using ::testing::ElementsAre;

TEST(chord_metadata_manager, constructor_initializes_database) {
  Context context;

  std::string meta_dir = context.meta_directory;
  if(file::exists(meta_dir)) file::remove_all(meta_dir);

  fs::MetadataManager metadata{context};
  ASSERT_TRUE(chord::file::is_directory(meta_dir));
}

TEST(chord_metadata_manager, set_and_get) {
  Context context;

  fs::MetadataManager metadata{context};
  auto uri = uri::from("chord:/folder");
  fs::Metadata meta_set;
  meta_set.name = "file1";

  metadata.add(uri, {meta_set});
  set<fs::Metadata> meta_get = metadata.get(uri);

  fs::Metadata expected{"file1"};
  //ASSERT_EQ(meta_get.name, "/folder");
  ASSERT_THAT(meta_get, ElementsAre(expected));
}

TEST(chord_metadata_manager, set_delete_get) {
  Context context;

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
}
