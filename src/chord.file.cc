#include <string>
#include <fstream>
#include <errno.h>
#include <sys/types.h>
#include <attr/xattr.h>
#include <experimental/filesystem>

#include "chord.exception.h"

#include "chord.file.h"

namespace chord {

using namespace std;
namespace fs = std::experimental::filesystem;

// wrappers
bool file::exists(const std::string& path) {
  return fs::exists({path});
}

bool file::remove(const std::string& path) {
  return fs::remove({path});
}

bool file::remove_all(const std::string& path) {
  return fs::remove_all({path});
}

bool file::create_file(const std::string& path) {
  std::ofstream created{fs::path(path)};
  return true;
}

bool file::create_directory(const std::string& path) {
  return fs::create_directory({path});
}

bool file::create_directories(const std::string& path) {
  return fs::create_directories({path});
}

bool file::has_attr(const std::string& path, const std::string& name) {
  size_t read = ::getxattr(path.c_str(), name.c_str(), nullptr, 0);
  if(read == -1u) return false;

  return true;
}

/// xattr get
attribute file::attr(const std::string& path, const std::string& name) {
  using namespace std::string_literals;
  size_t read = ::getxattr(path.c_str(), name.c_str(), nullptr, 0);
  if(read == -1u) return {false};

#if __cplusplus >= 201703L
  std::string value; value.resize(read);
  read = ::getxattr(path.c_str(), name.c_str(), value.data(), value.size());
#else
  char buffer[read];
  read = ::getxattr(path.c_str(), name.c_str(), buffer, read);
  std::string value(buffer, read);
#endif
  if(read == -1u) throw new chord::exception("failed to get xattr"s + strerror(errno));

  return {true, value};
}

/// xattr set
bool file::attr(const std::string& path, const std::string& name, const std::string& value) {
  using namespace std::string_literals;
  size_t err = ::setxattr(path.c_str(), name.c_str(), value.data(), value.size(), 0);
  if(err == -1u) throw new chord::exception("failed to set xattr"s + strerror(errno));

  return true;
}

bool file::attr_remove(const std::string& path, const std::string& name) {
  using namespace std::string_literals;
  size_t err = ::removexattr(path.c_str(), name.c_str());
  if(err == -1u) throw new chord::exception("failed to remove xattr: "s + strerror(errno));

  return true;
}
}
