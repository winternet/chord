#include "chord.path.h"

using namespace std;
namespace fs = std::experimental::filesystem;

namespace chord {

path::path(const fs::path &other)
    : _path{other} {}

//path::path(const chord::path &other)
//    : _path{other._path} {}

path path::filename() const { return path{_path.filename()}; }

path path::extension() const { return path{_path.extension()}; }

path path::parent_path() const { return path{_path.parent_path()}; }

std::string path::string() const { return _path.string(); }

path path::canonical() const {
  path result;
  for (const auto &dir : _path) {
    //for(auto it = begin(_path); it != end(_path); it++) {
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

path path::operator/=(const path &p) { return path{_path /= p._path}; }

bool path::operator==(const path &p) const { return _path==p._path; }

bool path::operator==(const std::string &s) const { return s==string(); }

bool operator==(const std::string &p1, const path &p2) {
  return p1==p2.string();
}

std::ostream &operator<<(std::ostream &os, const path &path) {
  return os << path._path;
}

} //namespace chord
