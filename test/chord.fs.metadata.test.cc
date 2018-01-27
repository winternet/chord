#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "chord.fs.metadata.h"

using namespace std;
using namespace chord;
using namespace chord::fs;

TEST(chord_metadata, output_empty_directory) {
  Metadata meta{"/", "usr", "grp"};
  meta.permissions = perms::all;
  meta.file_type = type::directory;

  stringstream ss;
  ss << meta;
  ASSERT_EQ(R"(/:
drwxrwxrwx usr grp .
)", ss.str());
}

TEST(chord_metadata, output_directory) {
  Metadata meta{"/", "usr", "grp"};
  meta.permissions = perms::all;
  meta.file_type = type::directory;
  meta.files = {{"foo", "usr", "grp", perms::owner_all}, {"bar", "usr2", "grp", perms::owner_all | perms::group_read}};

  stringstream ss;
  ss << meta;
  ASSERT_EQ(R"(/:
drwxrwxrwx usr  grp .
-rwxr----- usr2 grp bar
-rwx------ usr  grp foo
)", ss.str());
}
