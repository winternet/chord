#pragma once

#include <cstdint>
#include <experimental/optional>
#include <string>

namespace chord {

namespace file {
  // wrappers
  bool exists(const std::string &path);

  bool remove(const std::string &path);

  void rename(const std::string &from, const std::string &to);

  bool remove_all(const std::string &path);

  bool create_file(const std::string &path);

  bool copy_file(const std::string &from, const std::string& to);

  bool is_empty(const std::string& path);

  bool is_regular_file(const std::string &path);

  bool is_directory(const std::string &path);

  bool create_directory(const std::string &path);

  bool create_directories(const std::string &path);

  bool has_attr(const std::string &path, const std::string &name);

  bool files_equal(const std::string &file1, const std::string &file2);

  uintmax_t file_size(const std::string& path);

  /// xattr get
  std::experimental::optional<std::string> attr(const std::string &path, const std::string &name);

  /// xattr set
  bool attr(const std::string &path, const std::string &name, const std::string &value);

  bool attr_remove(const std::string &path, const std::string &name);
}  // namespace file

} //namespace chord
