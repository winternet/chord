#include <string>
#include <fstream>
#include <errno.h>
#include <sys/types.h>
#include <attr/xattr.h>
#include <experimental/filesystem>

#include "chord.exception.h"

namespace chord {

  struct attribute {
    bool success;
    std::string value;
    attribute(bool succ):success(succ) {};
    attribute(bool succ, std::string val):success(succ), value(val) {};
  };

  struct file {
    // wrappers
    static bool exists(const std::string& path) {
      return std::experimental::filesystem::exists({path});
    }

    static bool remove(const std::string& path) {
      return std::experimental::filesystem::remove({path});
    }

    static bool remove_all(const std::string& path) {
      return std::experimental::filesystem::remove_all({path});
    }

    static bool create_file(const std::string& path) {
      std::ofstream created{std::experimental::filesystem::path(path)};
      return true;
    }

    static bool create_directory(const std::string& path) {
      return std::experimental::filesystem::create_directory({path});
    }

    static bool create_directories(const std::string& path) {
      return std::experimental::filesystem::create_directories({path});
    }

    /// xattr get
    static attribute attr(const std::string& path, const std::string& name) {
      using namespace std::string_literals;
      size_t read = ::getxattr(path.c_str(), name.c_str(), nullptr, 0);
      if(read == -1u) return {false};

      std::string value; value.resize(read);

      read = ::getxattr(path.c_str(), name.c_str(), value.data(), value.size());
      if(read == -1u) throw new chord::exception("failed to get xattr"s + strerror(errno));

      return {true, value};
    }

    /// xattr set
    static bool attr(const std::string& path, const std::string& name, const std::string& value) {
      using namespace std::string_literals;
      size_t err = ::setxattr(path.c_str(), name.c_str(), value.data(), value.size(), 0);
      if(err == -1u) throw new chord::exception("failed to set xattr"s + strerror(errno));

      return true;
    }

    static bool attr_remove(const std::string& path, const std::string& name) {
      using namespace std::string_literals;
      size_t err = ::removexattr(path.c_str(), name.c_str());
      if(err == -1u) throw new chord::exception("failed to remove xattr: "s + strerror(errno));

      return true;
    }
  };
};
