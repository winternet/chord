#pragma once

#include <map>
#include <memory>
#include <set>
#include <string>

#include <spdlog/spdlog.h>
//#include <spdlog/fmt/bundled/ostream.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/rotating_file_sink.h>

#include "chord.log.factory.h"

namespace spdlog { class logger; }

namespace chord {

namespace log {

struct Formatter {
  std::string pattern;

  bool operator<(const Formatter& other) const;
  bool operator==(const Formatter& other) const;
};

struct SinkType final {

  private:
  std::string mappedType;

  SinkType(std::string mappedType) noexcept;
  static std::set<SinkType>& types();

  public:
  static SinkType CONSOLE;
  static SinkType FILE_ROTATING;
  static SinkType FILE_DAILY;
  static SinkType FILE;

  static SinkType from(std::string str);

  bool operator==(const SinkType& other) const;
  bool operator!=(const SinkType& other) const;
  bool operator<(const SinkType& other) const;

};


struct Sink {
  SinkType type = SinkType::CONSOLE;
  //optional
  std::string path;
  Formatter* formatter;
  std::string level;

  bool operator<(const Sink& other) const;
};

struct Logger {
  std::set<Sink*> sinks;
  std::string filter;
  std::string level;
};

struct Logging {
  std::string level;

  std::map<std::string, Formatter> formatters;
  std::map<std::string, Sink> sinks;
  std::map<std::string, Logger> loggers;

  Factory factory() const;
};


// NOTE this might be slow since get and create locks the registry
//     with a mutex
std::shared_ptr<spdlog::logger> get_or_create(std::string name);


}  // namespace log
}  // namespace chord
