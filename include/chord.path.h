#pragma once

#include <experimental/filesystem>
#include <string>

namespace chord {

class path {

 private:
  std::experimental::filesystem::path _path;

 public:
  path() = default;

  path(const chord::path &other) = default;

  path(const std::experimental::filesystem::path &path);

  virtual ~path() = default;

  path canonical() const;

  path filename() const;

  path extension() const;

  path parent_path() const;

  std::vector<path> contents() const;

  std::string string() const;

  bool empty() const noexcept ;

  operator std::string() const;

  path operator/=(const path &p);

  bool operator==(const path &p) const;

  bool operator==(const std::string &s) const;

  friend bool operator==(const std::string &p1, const path &p2);

  friend std::ostream &operator<<(std::ostream &os, const path &path);
};

} //namespace chord
