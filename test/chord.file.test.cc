#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <iomanip>

#include "chord.file.h"

using namespace std;

TEST(chord_file, getxattr) {
  auto file = "xattr.get";
  auto attr_name = "user.chord.link";

  if(chord::file::exists(file)) chord::file::remove(file);

  //--- assert no attrs set on new file & nothing to remove
  chord::file::create_file(file);
  ASSERT_FALSE(chord::file::has_attr(file, attr_name));
  ASSERT_FALSE(chord::file::attr_remove(file, attr_name));

  //--- assert set xattr
  chord::file::attr(file, attr_name, "thevalue");
  ASSERT_TRUE(chord::file::has_attr(file, attr_name));

  //--- assert get xattr
  auto value = chord::file::attr(file, attr_name);
  ASSERT_EQ("thevalue"s, value);

  //--- assert remove xattr
  ASSERT_TRUE(chord::file::attr_remove(file, attr_name));
  ASSERT_FALSE(chord::file::has_attr(file, attr_name));

  ASSERT_TRUE(chord::file::exists(file));
  chord::file::remove(file);
  ASSERT_FALSE(chord::file::exists(file));
}

TEST(chord_file, create_directory) {
  auto dir = "testdir";

  if(chord::file::exists(dir)) chord::file::remove(dir);

  ASSERT_TRUE(chord::file::create_directory(dir));
  ASSERT_TRUE(chord::file::exists(dir));

  ASSERT_TRUE(chord::file::remove(dir));
  ASSERT_FALSE(chord::file::exists(dir));
}

TEST(chord_file, create_directories) {
  auto dir = "testdir/subfolder";

  if(chord::file::exists(dir)) chord::file::remove_all(dir);

  ASSERT_TRUE(chord::file::create_directories(dir));
  ASSERT_TRUE(chord::file::exists(dir));

  ASSERT_TRUE(chord::file::remove_all(dir));
  ASSERT_FALSE(chord::file::exists(dir));
}
