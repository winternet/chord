#include <yaml-cpp/yaml.h>
#include <functional>

#include "chord.log.h"
#include "chord.context.h"
#include "chord.context.manager.h"

namespace chord {
template<class T>
bool read(const YAML::Node& node, std::string key, T& target) {
  if(const YAML::Node n = node[key]) {
    target = n.as<T>();
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
} // namespace chord

namespace YAML {
/**
 * specialization for chord::uuid
 */
template <>
struct convert<chord::uuid> {
  static bool decode(const Node& node, chord::uuid& rhs) {
    rhs = uuid_t{node.as<std::string>()};
    return true;
  }
};

/**
 * specialization for chord::path
 */
template <>
struct convert<chord::path> {
  static bool decode(const Node& node, chord::path& rhs) {
    rhs = {node.as<std::string>()};
    return true;
  }
};

/**
 * specialization for std::set
 */
template <class T>
struct convert<std::set<T>> {
  static bool decode(const Node& node, std::set<T>& rhs) {
    if(node.IsSequence()) {
      for(const auto& elem:node) {
        rhs.insert(elem.as<T>());
      }
      return true;
    }
    return false;
  }
};

/**
 * specialization for Logging
 */
template <>
struct convert<chord::log::Logging> {
  static bool decode(const Node& node, chord::log::Logging& rhs) {
    chord::read(node, "formatters", rhs.formatters);

    chord::read(node, "sinks", rhs.sinks);
    for(const auto& pair: node["sinks"]) {
      const auto& sink_name = pair.first.as<std::string>();
      std::string formatter_name;
      if(chord::read(pair.second, "formatter", formatter_name)) {
        const auto it = rhs.formatters.find(formatter_name);
        if(it == rhs.formatters.end()) {
          spdlog::error("Failed to find formatter: {} for sink {}", formatter_name, sink_name);
          continue;
        }
        rhs.sinks[sink_name].formatter = &(it->second);
      }
    }

    chord::read(node, "loggers", rhs.loggers);
    for(const auto& pair: node["loggers"]) {
      const auto& logger_name = pair.first.as<std::string>();
      std::set<std::string> sinks;
      if(chord::read(pair.second, "sinks", sinks)) {
        for(const auto& sink:sinks) {
          const auto it = rhs.sinks.find(sink);
          if(it == rhs.sinks.end()) {
            spdlog::error("Failed to find sink: {} for logger {}", sink, logger_name);
            continue;
          }
          rhs.loggers[logger_name].sinks.insert(&(it->second));
        }
      }
    }
    return true;
  }
};

/**
 * specialization for Formatter
 */
template <>
struct convert<chord::log::Formatter> {
  static bool decode(const Node& node, chord::log::Formatter& rhs) {
    chord::read(node, "pattern", rhs.pattern);
    return true;
  }
};

/**
 * specialization for Sinks
 */
template <>
struct convert<chord::log::Sink> {
  static bool decode(const Node& node, chord::log::Sink& rhs) {
    chord::read(node, "path", rhs.path);
    rhs.type = chord::log::SinkType::from(node["type"].as<std::string>());
    return true;
  }
};

/**
 * specialization for Loggers
 */
template <>
struct convert<chord::log::Logger> {
  static bool decode(const Node& node, chord::log::Logger& rhs) {
    chord::read(node, "filter", rhs.filter);
    return true;
  }
};

}  // namespace YAML

namespace chord {

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

  read(node, "logging", context.logging);
}

void ContextManager::parse_v1(const YAML::Node& config, Context& context) {
  config >> context;
  Context::validate(context);
}

  Context ContextManager::load(const std::string& yaml) {
    Context context;
    YAML::Node config = YAML::Load(yaml);
    parse(config, context);
    return context;
  }
  Context ContextManager::load(const chord::path& path) {
    Context context;
    YAML::Node config = YAML::LoadFile(path);
    parse(config, context);
    return context;
  }
  void ContextManager::parse(const YAML::Node& root, Context& context) {
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
}

