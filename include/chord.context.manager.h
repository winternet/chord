#pragma once

#include <yaml-cpp/yaml.h>
#include <functional>
#include <string>
#include <vector>

#include "chord.context.h"
#include "chord.exception.h"
#include "chord.path.h"

namespace YAML {
/**
 * specialization for chord::uuid
 */
template <>
struct convert<chord::uuid> {
  static Node encode(const chord::uuid& rhs) {
    Node node;
    node = rhs.string();
    return node;
  }

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
  static Node encode(const chord::path& rhs) {
    Node node;
    node = rhs.string();
    return node;
  }

  static bool decode(const Node& node, chord::path& rhs) {
    rhs = {node.as<std::string>()};
    return true;
  }
};
}  // namespace YAML

namespace chord {

struct ContextManager {

  struct AbstractToken {
    std::string name;

    explicit AbstractToken(std::string name) : name{std::move(name)} {};
    virtual ~AbstractToken() = default;

    virtual void set(const YAML::Node& config) = 0;
  };

  template <class T>
  struct Token : public AbstractToken {
    using setter_type = std::function<void(Context&, const T&)>;

    T& value;
    std::function<void(Context&, const T&)> setter;

    Token(std::string name, T& value) : AbstractToken{name}, value{value} {};
    //cppcheck-suppress uninitMemberVar
    Token(std::string name, std::function<void(Context&, const T&)>&& setter)
        : AbstractToken{name}, setter{setter} {};
    void set(const YAML::Node& config) override {
      value = config[name].as<T>();
    }
  };

  template <class T>
  struct SetterToken : public AbstractToken {
    using setter_type = std::function<void(Context&, const T&)>;

    Context& context;
    std::function<void(Context&, const T&)> setter;

    SetterToken(std::string name, Context& context,
                std::function<void(Context&, const T&)>&& setter)
        : AbstractToken{name}, context{context}, setter{setter} {};
    void set(const YAML::Node& config) override {
      setter(context, config[name].as<T>());
    }
  };

  static void parse_v1(const YAML::Node& config, Context& context) {
    std::vector<std::unique_ptr<AbstractToken> > tokens;
    tokens.emplace_back(new Token<chord::path>{"data-directory", context.data_directory});
    tokens.emplace_back(new Token<chord::path>{"meta-directory", context.meta_directory});
    tokens.emplace_back(new Token<endpoint_t>{"bind-addr", context.bind_addr});
    tokens.emplace_back(new Token<endpoint_t>{"join-addr", context.join_addr});

    tokens.emplace_back(new Token<bool>{"bootstrap", context.bootstrap});
    tokens.emplace_back(new Token<bool>{"no-controller", context.no_controller});

    tokens.emplace_back(new Token<size_t>{"stabilize-ms", context.stabilize_period_ms});
    tokens.emplace_back(new Token<size_t>{"check-ms", context.check_period_ms});

    tokens.emplace_back(new SetterToken<chord::uuid>{"uuid", context, &Context::set_uuid});

    for (auto it = config.begin(); it != config.end(); ++it) {
      const auto key = it->first.as<std::string>();
      for (auto& token : tokens) {
        if (token->name == key) {
          token->set(config);
        }
      }
      std::cerr << "\nnode: " << it->first.as<std::string>() << " is "
                << it->second.as<std::string>();
    }
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
