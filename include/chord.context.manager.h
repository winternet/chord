#pragma once

//#include <yaml-cpp/yaml.h>
//#include <functional>
#include <string>
//#include <vector>

#include "chord.context.h"
//#include "chord.exception.h"
#include "chord.path.h"
//#include "chord.log.h"

namespace YAML { class Node; }

namespace chord {

struct ContextManager {

  static void parse_v1(const YAML::Node& config, Context& context);

  static Context load(const std::string& yaml);

  static Context load(const chord::path& path);

  static void parse(const YAML::Node& root, Context& context);

};

}  // namespace chord
