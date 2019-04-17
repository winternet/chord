#pragma once

#include <yaml-cpp/yaml.h>
#include <functional>
#include <string>
#include <vector>

#include "chord.context.h"
#include "chord.exception.h"
#include "chord.path.h"
#include "chord.context.logging.h"



namespace chord {

template<class T>
void operator>>(const YAML::Node& node, T& val) {
  val = node.as<T>();
}

void operator>>(const YAML::Node& node, chord::path& p) {
  p = chord::path(node.as<std::string>());
}

void operator>>(const YAML::Node& node, uuid_t& uuid) {
  uuid = chord::uuid(node.as<std::string>());
}

template<class T>
bool read(const YAML::Node& node, std::string key, T& target) {
  if(const YAML::Node n = node[key]) {
    n >> target;
    return true;
  }
  return false;
}
template<class T>
void set(const YAML::Node& node, std::string key, std::function<void(T)> setter) {
  T val;
  if(read(node, key, val)) {
    setter(val);
  }
}

void operator>>(const YAML::Node& node, Context& context) {
  read(node, "data-directory", context.data_directory);
  read(node, "meta-directory", context.meta_directory);
  read(node, "bind-addr", context.bind_addr);
  read(node, "join-addr", context.join_addr);
  read(node, "bootstrap", context.bootstrap);
  read(node, "no-controller", context.no_controller);
  read(node, "stabilize-ms", context.stabilize_period_ms);
  read(node, "check-ms", context.check_period_ms);
  read(node, "replication-count", context.replication_cnt);

  set(node, "uuid", std::function<void(uuid_t)>([&](auto v) {context.set_uuid(v);}));
}

struct ContextManager {
  static void parse_v1(const YAML::Node& config, Context& context) {
    config >> context;
    Context::validate(context);
  }

  static Context load(const std::string& yaml) {
    Context context;
    YAML::Node config = YAML::Load(yaml);
    parse(config, context);
    return context;
  }

  static Context load(const chord::path& path) {
    Context context;
    YAML::Node config = YAML::LoadFile(path);
    parse(config, context);
    return context;
  }

  static void parse(const YAML::Node& root, Context& context) {
    const auto version = root["version"].as<int>();
    switch (version) {
      case 1:
        parse_v1(root, context);
        break;
      default:
        throw__exception(
            std::string{
                "Failed to prase configuration file: unknown version ("} +
            std::to_string(version) + std::string(")"));
    }
  }

};

}  // namespace chord
