#pragma once

#include <string>
#include <experimental/optional>

namespace chord {

struct file {
  // wrappers
  static bool exists(const std::string &path);

  static bool remove(const std::string &path);

  static bool remove_all(const std::string &path);

  static bool create_file(const std::string &path);

  static bool is_regular_file(const std::string &path);

  static bool is_directory(const std::string &path);

  static bool create_directory(const std::string &path);

  static bool create_directories(const std::string &path);

  static bool has_attr(const std::string &path, const std::string &name);

  /// xattr get
  static std::experimental::optional<std::string> attr(const std::string &path, const std::string &name);

  /// xattr set
  static bool attr(const std::string &path, const std::string &name, const std::string &value);

  static bool attr_remove(const std::string &path, const std::string &name);
};

} //namespace chord
