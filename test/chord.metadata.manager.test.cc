#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "chord.file.h"
#include "chord.metadata.manager.h"

using namespace std;
using namespace chord;
using namespace chord::fs;
using ::testing::ElementsAre;

TEST(chord_metadata_manager, permissions) {
   ASSERT_EQ(perms::owner_all, (perms::owner_read | perms::owner_write | perms::owner_exec));
   ASSERT_EQ(perms::group_all, (perms::group_read | perms::group_write | perms::group_exec));
   ASSERT_EQ(perms::others_all,(perms::others_read | perms::others_write | perms::others_exec));

   ASSERT_EQ(perms::none, ((perms::owner_read & perms::owner_write & perms::owner_exec)
                          &(perms::group_read & perms::group_write & perms::group_exec)
                          &(perms::others_read & perms::others_write & perms::others_exec)));
}

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

  metadata.add(uri, meta_set);
  fs::Metadata meta_get = metadata.get(uri);

  fs::Metadata expected{"file1"};
  ASSERT_EQ(meta_get.name, "/folder");
  ASSERT_THAT(meta_get.files, ElementsAre(expected));
}

TEST(chord_metadata_manager, set_delete_get) {
  Context context;

  fs::MetadataManager metadata{context};
  auto uri = uri::from("chord:/folder");
  fs::Metadata meta_set;
  meta_set.name = "file1";

  metadata.add(uri, meta_set);
  metadata.del(uri);
  try {
    metadata.get(uri);
    FAIL();
  } catch(const chord::exception &expected) {}
}
