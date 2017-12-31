#pragma once

#include <yaml-cpp/yaml.h>

#include "chord.context.h"

namespace chord {

struct ContextManager {
  static Context load(const chord::path& path) {
    YAML::Node config = YAML::LoadFile(path);
    return Context();
  }

};

} //namespace chord
