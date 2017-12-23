#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "chord.path.h"

using namespace std;
using namespace chord;

TEST(chord_path, canonical) {
  ASSERT_EQ(path{"/folder/subfolder/bar.ext"}, path{"/folder/subfolder/././///../subfolder/bar.ext"}.canonical());
  ASSERT_EQ(path{"/folder/subfolder/bar.ext"}, path{"/folder/subfolder/bar.ext"}.canonical());
}

TEST(chord_path, compare_path_with_string) {
  ASSERT_EQ(path{"/folder/subfolder/bar.ext"}, "/folder/subfolder/bar.ext");
  ASSERT_EQ(path{"/folder/subfolder/bar.ext"}.string(), "/folder/subfolder/bar.ext");
}

TEST(chord_path, compare_string_with_path) {
  ASSERT_EQ("/folder/subfolder/bar.ext", path{"/folder/subfolder/bar.ext"});
  ASSERT_EQ("/folder/subfolder/bar.ext", path{"/folder/subfolder/bar.ext"}.string());
}

TEST(chord_path, parent_path) {
  ASSERT_EQ("/folder/subfolder", path{"/folder/subfolder/bar.ext"}.parent_path());
  ASSERT_EQ("/folder/subfolder", path{"/folder/subfolder/"}.parent_path());
  ASSERT_EQ("/folder", path{"/folder/subfolder"}.parent_path());
}

TEST(chord_path, filename) {
  ASSERT_EQ("bar.ext", path{"/folder/subfolder/bar.ext"}.filename());
  ASSERT_EQ("foo", path{"/folder/subfolder/foo"}.filename());
  ASSERT_EQ(".", path{"/folder/subfolder/"}.filename());
}

TEST(chord_path, extension) {
  ASSERT_EQ(".ext", path{"/folder/subfolder/bar.ext"}.extension());
  ASSERT_EQ(""s, path{"/folder/subfolder/foo"}.extension());
}