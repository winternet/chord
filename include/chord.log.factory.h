#pragma once

#include "chord.singleton.h"

#include <string>
#include <memory>

namespace spdlog {
  class logger;
  namespace sinks {
    class sink;
  }
}

namespace chord {
namespace log {

struct Logging;
struct Logger;
struct Sink;

class Factory final : public Singleton<Factory> {
  private:
    const Logging& logging;

    std::shared_ptr<spdlog::sinks::sink> create_sink(const Sink* sink) const;
    std::shared_ptr<spdlog::logger> create_logger(const std::string& name, const Logger& logger) const;

    bool valid_level(const std::string level) const;
    bool prepare_file_sink(const std::string& file) const;

    template<class T>
    std::shared_ptr<T> configure_sink(std::shared_ptr<T>, const Sink* ) const;

  public:
    Factory(const Logging& logging);

    std::shared_ptr<spdlog::logger> get_or_create(std::string name);
};

}  // namespace log
}  // namespace chord
