#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "chord.fs.metadata.h"

using namespace std;
using namespace chord;
using namespace chord::fs;

TEST(chord_metadata, operator_less) {
  Metadata meta1{"/data1", "usr", "grp", perms::none, type::regular};
  Metadata meta2{"/data1", "usr", "grp", perms::none, type::regular};

  set<Metadata> equal = {meta1, meta2};
  ASSERT_EQ(equal.size(), 1);
}

TEST(chord_metadata, operator_less_2) {
  Metadata meta1{"/data1", "usr", "grp", perms::none, type::regular};
  Metadata meta2{"/data2", "usr", "grp", perms::none, type::regular};

  set<Metadata> unequal = {meta1, meta2};
  ASSERT_EQ(unequal.size(), 2);
}

TEST(chord_metadata, output_empty_directory) {
  Metadata meta{"/", "usr", "grp", perms::all, type::directory};

  stringstream ss;
  ss << meta;
  ASSERT_EQ(R"(drwxrwxrwx usr grp 1  /)", ss.str());
}

TEST(chord_metadata, output_directory) {
  set<Metadata> contents(
      {{"/", "usr", "grp", perms::all, type::directory, 0, {}, {}, Replication::ALL},
       {"foo", "usr", "grp", perms::owner_all, type::regular, 33, {}, {}, Replication::ALL},
       {"bar", "usr2", "grp", perms::owner_all | perms::group_read, type::regular, 33, {}, {}, Replication::ALL}});

  stringstream ss;
  ss << contents;
  ASSERT_EQ(R"(drwxrwxrwx usr  grp ∞  /
-rwxr----- usr2 grp ∞  bar
-rwx------ usr  grp ∞  foo
)", ss.str());
}

TEST(chord_metadata, create_directory_blank) {
  const auto meta = create_directory();
  ASSERT_EQ(meta.name, ".");
  ASSERT_EQ(meta.file_type, type::directory);
  ASSERT_EQ(meta.replication, Replication::NONE);
}

TEST(chord_metadata, create_directory) {
  std::set<Metadata> contents = {
    Metadata("name1", perms::all, 7, {}, {}, Replication(0,3)),
    Metadata("name2", perms::all, 7, {}, {}, Replication(2,5))
  };
  const auto meta = create_directory(contents);
  ASSERT_EQ(meta.name, ".");
  ASSERT_EQ(meta.file_type, type::directory);

  const auto expected_repl = Replication(0,5);
  ASSERT_EQ(meta.replication, expected_repl) 
    << "Expected equality of expected " << expected_repl.string() 
    << " and actual " << (meta.replication ? meta.replication.string() : "<empty>");
}
