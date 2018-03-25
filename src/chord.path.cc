#include "chord.path.h"
#include "chord.file.h"

using namespace std;
namespace fs = std::experimental::filesystem;

namespace chord {

path::path(const fs::path &other)
    : _path{other} {}

path path::filename() const { return path{_path.filename()}; }

path path::extension() const { return path{_path.extension()}; }

path path::parent_path() const { return path{_path.parent_path()}; }

std::set<path> path::contents() const {
  std::set<path> files_and_dirs;
  if (chord::file::is_regular_file(_path)) {
    return files_and_dirs;
  }

  for (const auto& entity : std::experimental::filesystem::directory_iterator(_path)) {
    files_and_dirs.emplace(entity);
  }
	return files_and_dirs;
}

std::string path::string() const { return _path.string(); }

set<path> path::all_paths() const {
  set<path> paths;
  path current_path;
  for (const auto &dir : canonical()._path) {
    current_path /= dir;
    paths.emplace(current_path);
  }
  return paths;
}

path path::canonical() const {
  path result;
  for (const auto &dir : _path) {
    if (dir=="..") {
      result = result.parent_path();
    } else if (dir==".") {
      //ignore
    } else {
      result /= dir;
    }
  }
  return result;
}

bool path::empty() const noexcept {
  return _path.empty();
}

path::operator std::string() const { return _path.string(); }

path path::operator/=(const path &p) { return path{_path /= p._path}; }

path path::operator/(const path &p) const { return path{_path / p._path}; }

bool path::operator==(const path &p) const { return _path==p._path; }

bool path::operator<(const path &p) const {
  return _path < p._path;
}

bool path::operator>(const path &p) const {
  return _path > p._path;
}

bool path::operator==(const std::string &s) const { return s==string(); }

bool operator==(const std::string &p1, const path &p2) {
  return p1==p2.string();
}

std::ostream &operator<<(std::ostream &os, const path &path) {
  return os << path.string();
}

} //namespace chord
