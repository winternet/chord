#pragma once

#include <string>
#include <experimental/filesystem>

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

  std::string string() const;

  bool empty() const noexcept ;

  operator std::string() const;

  path operator/=(const path &p);// { return path{_path /= p._path}; }

  bool operator==(const path &p) const;// { return _path == p._path; }

  bool operator==(const std::string &s) const;// { return s == string(); }

  friend bool operator==(const std::string &p1, const path &p2);

  friend std::ostream &operator<<(std::ostream &os, const path &path);
};

} //namespace chord
