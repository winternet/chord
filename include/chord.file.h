#pragma once

#include <experimental/optional>
#include <string>

namespace chord {

namespace file {
  // wrappers
  bool exists(const std::string &path);

  bool remove(const std::string &path);

  bool remove_all(const std::string &path);

  bool create_file(const std::string &path);

  bool is_regular_file(const std::string &path);

  bool is_directory(const std::string &path);

  bool create_directory(const std::string &path);

  bool create_directories(const std::string &path);

  bool has_attr(const std::string &path, const std::string &name);

  /// xattr get
  std::experimental::optional<std::string> attr(const std::string &path, const std::string &name);

  /// xattr set
  bool attr(const std::string &path, const std::string &name, const std::string &value);

  bool attr_remove(const std::string &path, const std::string &name);
}  // namespace file

} //namespace chord
