#pragma once
#include <string>

struct project {
  static constexpr std::string name{"@PROJECT@"};
  static constexpr std::string version{"@PROJECT_VERSION@"};
  static constexpr short version_major{@PROJECT_VERSION_MAJOR@};
  static constexpr short version_minor{@PROJECT_VERSION_MINOR@};
  static constexpr short version_patch{@PROJECT_VERSION_PATCH@};
};
