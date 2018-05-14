#include <string>
#include <fstream>
#include <attr/xattr.h>
#include <experimental/filesystem>

#include "chord.exception.h"

#include "chord.file.h"

namespace chord {

using namespace std;
namespace fs = std::experimental::filesystem;

// wrappers
bool file::exists(const std::string &path) {
  return fs::exists({path});
}

bool file::is_regular_file(const std::string &path) {
  return fs::is_regular_file({path});
}

bool file::is_directory(const std::string &path) {
  return fs::is_directory({path});
}

bool file::remove(const std::string &path) {
  return fs::remove({path});
}

bool file::remove_all(const std::string &path) {
  return fs::remove_all({path});
}

bool file::create_file(const std::string &path) {
  std::ofstream{fs::path(path)};
  return true;
}

bool file::create_directory(const std::string &path) {
  return fs::create_directory({path});
}

bool file::create_directories(const std::string &path) {
  return fs::create_directories({path});
}

bool file::has_attr(const std::string &path, const std::string &name) {
  size_t read = ::getxattr(path.c_str(), name.c_str(), nullptr, 0);
  if (read==static_cast<size_t>(-1)) return false;

  return true;
}

/// xattr get
std::experimental::optional<std::string> file::attr(const std::string &path, const std::string &name) {
  using namespace std::string_literals;
  size_t read = ::getxattr(path.c_str(), name.c_str(), nullptr, 0);
  if (read==static_cast<size_t>(-1)) return {};

#if __cplusplus >= 201703L
  std::string value; value.resize(read);
  read = ::getxattr(path.c_str(), name.c_str(), value.data(), value.size());
#else
  char *buffer = new char[read];
  read = ::getxattr(path.c_str(), name.c_str(), buffer, read);
  std::string value(buffer, read);
  delete[] buffer;
#endif
  if (read==static_cast<size_t>(-1)) throw__exception("failed to get xattr"s + strerror(errno));

  return value;
}

/// xattr set
bool file::attr(const std::string &path, const std::string &name, const std::string &value) {
  using namespace std::string_literals;
  size_t err = ::setxattr(path.c_str(), name.c_str(), value.data(), value.size(), 0);
  if (err==static_cast<size_t>(-1)) throw__exception("failed to set xattr"s + strerror(errno));

  return true;
}

bool file::attr_remove(const std::string &path, const std::string &name) {
  using namespace std::string_literals;
  size_t err = ::removexattr(path.c_str(), name.c_str());
  if (err==static_cast<size_t>(-1)) return false;

  return true;
}

} //namespace chord
