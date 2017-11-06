#pragma once

#include <string>

namespace chord {

  struct attribute {
    bool success;
    std::string value;
    attribute(bool succ):success(succ) {};
    attribute(bool succ, std::string val):success(succ), value(val) {};
  };

  struct file {
    // wrappers
    static bool exists(const std::string& path);

    static bool remove(const std::string& path);

    static bool remove_all(const std::string& path);

    static bool create_file(const std::string& path);

    static bool create_directory(const std::string& path);

    static bool create_directories(const std::string& path);

    /// xattr get
    static attribute attr(const std::string& path, const std::string& name);

    /// xattr set
    static bool attr(const std::string& path, const std::string& name, const std::string& value);

    static bool attr_remove(const std::string& path, const std::string& name);
  };
};
