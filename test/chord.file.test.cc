#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "chord.file.h"
#include "util/chord.test.tmp.dir.h"
#include "util/chord.test.tmp.file.h"

using namespace std;
using namespace chord::test;

using chord::path;

TEST(DISABLED_chord_file, getxattr) {
  auto file = "xattr.get";
  auto attr_name = "user.chord.link";

  if (chord::file::exists(file)) chord::file::remove(file);

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
  auto dir = "testdir"s;

  if (chord::file::exists(dir)) chord::file::remove(dir);

  ASSERT_TRUE(chord::file::create_directory(dir));
  ASSERT_TRUE(chord::file::exists(dir));
  ASSERT_TRUE(chord::file::exists(dir+"/."));
  ASSERT_TRUE(chord::file::exists(path(dir) / "."));
  ASSERT_TRUE(chord::file::is_empty(dir));
  ASSERT_FALSE(chord::file::is_regular_file(dir));

  ASSERT_TRUE(chord::file::remove(dir));
  ASSERT_FALSE(chord::file::exists(dir));
}

TEST(chord_file, create_directories) {
  auto dir = "testdir/subfolder";

  if (chord::file::exists(dir)) chord::file::remove_all(dir);

  ASSERT_TRUE(chord::file::create_directories(dir));
  ASSERT_TRUE(chord::file::exists(dir));

  ASSERT_TRUE(chord::file::remove_all(dir));
  ASSERT_FALSE(chord::file::exists(dir));
}

TEST(chord_file, files_equal) {
  TmpDir tmpDir;
  {
    TmpFile tmpFile1(path(tmpDir) / "file1");
    TmpFile tmpFile2(path(tmpDir) / "file2");
    ASSERT_FALSE(chord::file::files_equal(path(tmpFile1), path(tmpFile2)));
  }
  {
    TmpFile tmpFile1(path(tmpDir) / "file1");
    TmpFile tmpFile2(path(tmpDir) / "file2");
    ofstream file1(tmpFile1.path, std::ofstream::trunc|std::ofstream::binary);
    ofstream file2(tmpFile2.path, std::ofstream::trunc|std::ofstream::binary);
    file1 << "abcdefgh";
    file2 << "abcdefgh";
    ASSERT_TRUE(chord::file::files_equal(path(tmpFile1), path(tmpFile2)));
  }
  ASSERT_FALSE(chord::file::files_equal(path("__not_existant1__"), path("__not_existant2__")));
  ASSERT_FALSE(chord::file::files_equal(path("."), path(".")));
}

TEST(chord_file, rename) {
  TmpDir fromDir;
  TmpDir toDir;
  const auto file = "file.txt";
  {
    TmpFile tmpFile(fromDir.path /file);
    ASSERT_TRUE(chord::file::exists(fromDir.path/file));

    chord::file::rename(tmpFile.path, toDir.path/file);
    ASSERT_TRUE(chord::file::exists(toDir.path/file));
    ASSERT_FALSE(chord::file::exists(fromDir.path/file));
  }
}
