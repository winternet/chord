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
  ASSERT_EQ(R"(drwxrwxrwx usr grp /)", ss.str());
}

TEST(chord_metadata, output_directory) {
  set<Metadata> contents(
      {{"/", "usr", "grp", perms::all, type::directory},
       {"foo", "usr", "grp", perms::owner_all, type::regular},
       {"bar", "usr2", "grp", perms::owner_all | perms::group_read, type::regular}});

  stringstream ss;
  ss << contents;
  ASSERT_EQ(R"(drwxrwxrwx usr  grp /
-rwxr----- usr2 grp bar
-rwx------ usr  grp foo
)", ss.str());
}
