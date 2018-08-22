#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "chord.file.h"
#include "chord.path.h"

using namespace std;
using namespace chord;

using ::testing::UnorderedElementsAre;
using ::testing::ElementsAre;

TEST(chord_path, canonical) {
  ASSERT_EQ(path{"/folder/subfolder/bar.ext"}, path{"/folder/subfolder/././///../subfolder/bar.ext"}.canonical());
  ASSERT_EQ(path{"/folder/subfolder/bar.ext"}, path{"/folder/subfolder/bar.ext"}.canonical());
  ASSERT_EQ(path{"/folder/subfolder/bar.ext"}, path{"/folder//subfolder/bar.ext"}.canonical());
  ASSERT_EQ(path{"/folder/subfolder/bar.ext"}, path{"/folder//subfolder/////bar.ext"}.canonical());
  ASSERT_EQ(path{"/folder/subfolder/bar.ext"}, path{"//folder//subfolder/////bar.ext"}.canonical());
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
  ASSERT_EQ("", path{"/"}.parent_path());
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

TEST(chord_path, all_directories) {
  set<path> directories = path{"/folder/subfolder/bar.ext"}.all_paths();
  ASSERT_THAT(directories, ElementsAre(
      path{"/"},
      path{"/folder"},
      path{"/folder/subfolder"},
      path{"/folder/subfolder/bar.ext"}
      ));
}

TEST(chord_path, path_minus_operator) {
  ASSERT_EQ("/subsub/bar.ext", path{"/folder/sub/subsub/bar.ext"} - path{"/folder/sub"});
  ASSERT_EQ("/folder/sub/bar.ext", path{"/folder/sub/subsub/bar.ext"} - path{"subsub"});
  ASSERT_EQ("/folder/sub/bar.ext", path{"/folder/sub/subsub/bar.ext"} - path{"/subsub"});
  ASSERT_EQ("/subsub/bar.ext", path{"/folder/sub/subsub/bar.ext"} - path{"folder/sub"});
}

TEST(chord_path, path_less_operator) {
  ASSERT_FALSE(path{"/folder/sub/subsub/subsubsub/bar.ext"} < path{"/folder/sub/subsub/bar.ext"});
  ASSERT_FALSE(path{"/folder/sub/subsub/bar.ext"} < path{"/folder/sub/subsub/bar.ext"});
  ASSERT_TRUE(path{"/folder/sub/subsub"} < path{"/folder/sub/subsub/bar.ext"});
  ASSERT_TRUE(path{"/"} < path{"/folder/sub/subsub/bar.ext"});
}

TEST(chord_path, path_less_equal_operator) {
  ASSERT_FALSE(path{"/folder/sub/subsub/subsubsub/bar.ext"} <= path{"/folder/sub/subsub/bar.ext"});
  ASSERT_TRUE(path{"/folder/sub/subsub/bar.ext"} <= path{"/folder/sub/subsub/bar.ext"});
  ASSERT_TRUE(path{"/folder/sub/subsub"} <= path{"/folder/sub/subsub/bar.ext"});
  ASSERT_TRUE(path{"/"} <= path{"/folder/sub/subsub/bar.ext"});
}

TEST(chord_path, equal_operator) {
  ASSERT_FALSE(path{"/folder/sub/subsub/subsubsub/bar.ext"} == path{"/folder/sub/subsub/bar.ext"});
  ASSERT_TRUE(path{"/folder/sub/subsub/bar.ext"} == path{"/folder/sub/subsub/bar.ext"});
  ASSERT_FALSE(path{"/folder/sub/subsub"} == path{"/folder/sub/subsub/bar.ext"});
  ASSERT_FALSE(path{"/"} == path{"/folder/sub/subsub/bar.ext"});
}

TEST(chord_path, greater_operator) {
  ASSERT_TRUE(path{"/folder/sub/subsub/subsubsub/bar.ext"} > path{"/folder/sub/subsub/bar.ext"});
  ASSERT_FALSE(path{"/folder/sub/subsub/bar.ext"} > path{"/folder/sub/subsub/bar.ext"});
  ASSERT_FALSE(path{"/folder/sub/subsub"} > path{"/folder/sub/subsub/bar.ext"});
  ASSERT_FALSE(path{"/"} > path{"/folder/sub/subsub/bar.ext"});
}

TEST(chord_path, greater_equal_operator) {
  ASSERT_TRUE(path{"/folder/sub/subsub/subsubsub/bar.ext"} >= path{"/folder/sub/subsub/bar.ext"});
  ASSERT_TRUE(path{"/folder/sub/subsub/bar.ext"} >= path{"/folder/sub/subsub/bar.ext"});
  ASSERT_FALSE(path{"/folder/sub/subsub"} >= path{"/folder/sub/subsub/bar.ext"});
  ASSERT_FALSE(path{"/"} >= path{"/folder/sub/subsub/bar.ext"});
}

TEST(chord_path, path_append_slash_operator) {
  ASSERT_EQ("/folder/sub//subsub/bar.ext", path{"/folder/sub/"} / path{"/subsub/bar.ext"});
  ASSERT_EQ("/folder/sub/subsub/bar.ext", (path{"/folder/sub/"} / path{"/subsub/bar.ext"}).canonical());
}

TEST(chord_path, contents) {
  file::create_directories("./folder/a/b");
  file::create_file("./folder/file1.txt");
  file::create_file("./folder/file2.txt");

  auto directory_contents = path{"./folder"}.contents();

  file::remove_all("./folder");

	ASSERT_THAT(directory_contents, UnorderedElementsAre(
				path{"./folder/a"},
				path{"./folder/file1.txt"},
				path{"./folder/file2.txt"}));
}

TEST(chord_path, recursive_contents) {
  file::create_directories("./folder/a/b");
  file::create_file("./folder/a/b/file_ab.txt");
  file::create_file("./folder/file1.txt");
  file::create_file("./folder/file2.txt");

  auto directory_contents = path{"./folder"}.recursive_contents();

  file::remove_all("./folder");

	ASSERT_THAT(directory_contents, UnorderedElementsAre(
		path{"./folder/a"}, 
		path{"./folder/a/b"}, 
		path{"./folder/a/b/file_ab.txt"}, 
		path{"./folder/file1.txt"}, 
		path{"./folder/file2.txt"}));
}
