#pragma once

#include <iosfwd>
#include <filesystem>
#include <set>
#include <string>
#include <string_view>
#include <fmt/ostream.h>
#include <spdlog/fmt/ostr.h>

namespace chord {

class path {

 private:
  std::filesystem::path _path;

 public:
  path() = default;

  path(const chord::path &other) = default;

  path(std::filesystem::path path);

  virtual ~path() = default;

  std::set<path> all_paths() const;

  path canonical() const;

  path filename() const;

  path extension() const;

  path parent_path() const;

  std::set<path> recursive_contents() const;
  std::set<path> contents() const;

  std::string string() const;

  bool empty() const noexcept ;

  operator std::string() const;

  path operator/=(const path &p);

  path operator-(const path &p) const;

  path operator/(const path& p) const;

  path operator/(const std::string_view p) const;

  bool operator==(const path &p) const;
  bool operator!=(const path &p) const;

  bool operator<(const path &p) const;
  bool operator<=(const path &p) const;

  bool operator>(const path &p) const;
  bool operator>=(const path &p) const;

  bool operator==(const std::string &s) const;

  friend bool operator==(const std::string &p1, const path &p2);

  friend std::ostream &operator<<(std::ostream &os, const path &path);
};

} //namespace chord
 
template<> struct fmt::formatter<chord::path> : ostream_formatter {};
