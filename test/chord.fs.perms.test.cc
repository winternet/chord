#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "chord.fs.perms.h"

using namespace std;
using namespace chord;
using namespace chord::fs;

TEST(chord_permissions, output_none) {
  stringstream ss;
  ss << perms::none;
  ASSERT_EQ("---------", ss.str());
}

TEST(chord_permissions, output_all_owner) {
  stringstream ss;
  ss << perms::owner_all;
  ASSERT_EQ("rwx------", ss.str());
}

TEST(chord_permissions, output_all_group) {
  stringstream ss;
  ss << perms::group_all;
  ASSERT_EQ("---rwx---", ss.str());
}

TEST(chord_permissions, output_all_others) {
  stringstream ss;
  ss << perms::others_all;
  ASSERT_EQ("------rwx", ss.str());
}

TEST(chord_permissions, output_all) {
  stringstream ss;
  ss << perms::all;
  ASSERT_EQ("rwxrwxrwx", ss.str());
}

TEST(chord_permissions, permissions) {
   ASSERT_EQ(perms::owner_all, (perms::owner_read | perms::owner_write | perms::owner_exec));
   ASSERT_EQ(perms::group_all, (perms::group_read | perms::group_write | perms::group_exec));
   ASSERT_EQ(perms::others_all,(perms::others_read | perms::others_write | perms::others_exec));

   ASSERT_EQ(perms::none, ((perms::owner_read & perms::owner_write & perms::owner_exec)
                          &(perms::group_read & perms::group_write & perms::group_exec)
                          &(perms::others_read & perms::others_write & perms::others_exec)));
}
