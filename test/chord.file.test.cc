#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <iomanip>

#include "chord.file.h"

using namespace std;

TEST(chord_file, getxattr) {
  auto file = "xattr.get";
  auto attr_name = "user.chord.link";

  chord::file::create_file(file);
  chord::file::attr(file, attr_name, "thevalue");
#if __cplusplus >= 2017031L
  auto [success, attr_value] = chord::file::attr(file, attr_name);
#else
  auto attribute = chord::file::attr(file, attr_name);
  auto success = attribute.success;
  auto attr_value = attribute.value;
#endif

  ASSERT_TRUE(chord::file::exists(file));
  chord::file::remove(file);
  ASSERT_FALSE(chord::file::exists(file));

  ASSERT_TRUE(success);
  ASSERT_EQ("thevalue", attr_value);
}
