#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "chord.fs.metadata.builder.h"

using namespace std;

using chord::fs::Data;
using chord::fs::MetaRequest;
using chord::fs::perms;
using chord::fs::type;
using chord::fs::MetadataBuilder;
using chord::fs::value_of;

using ::testing::AllOf;
using ::testing::StartsWith;
using ::testing::EndsWith;

TEST(chord_fs_metadata_builder, from_data) {

  Data data;
  data.set_type(value_of(type::regular));
  data.set_filename("foo.txt");
  data.set_permission(value_of(perms::all));

  auto metadata = MetadataBuilder::from(data);

  ASSERT_EQ(metadata.name, "foo.txt");
  ASSERT_EQ(metadata.file_type, type::regular);
  ASSERT_EQ(metadata.permissions, perms::all);
}

TEST(chord_fs_metadata_builder, from_request) {
  MetaRequest req;
  auto *dir = req.mutable_metadata();
  dir->set_filename("folder");
  dir->set_type(value_of(type::directory));
  dir->set_permission(value_of(perms::all));

  auto *file1 = req.add_files();
  file1->set_filename("file1.txt");
  file1->set_type(value_of(type::regular));
  file1->set_permission(value_of(perms::all));

  auto *file2 = req.add_files();
  file2->set_filename("file2.txt");
  file2->set_type(value_of(type::regular));
  file2->set_permission(value_of(perms::all));

  //--- assert output correct
  auto meta = MetadataBuilder::from(&req);
  ASSERT_EQ(meta.name, "folder");
  ASSERT_EQ(meta.file_type, type::directory);
  ASSERT_EQ(meta.permissions, perms::all);

  ASSERT_EQ(meta.files.size(), 2);
  for(const auto &file : meta.files) {
    ASSERT_THAT(file.name, AllOf(StartsWith("file"), EndsWith(".txt")));
    ASSERT_EQ(file.file_type, type::regular);
    ASSERT_EQ(file.permissions, perms::all);
  }
}

TEST(chord_fs_metadata_builder, from_request_throws) {
  MetaRequest req;
  ASSERT_THROW(MetadataBuilder::from(&req), chord::exception);
}
