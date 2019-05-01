#pragma once

#include "chord.file.h"
#include "chord.log.factory.h"

#include <map>
#include <set>
#include <vector>
#include <spdlog/spdlog.h>
#include <spdlog/fmt/bundled/ostream.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/rotating_file_sink.h>


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

  SinkType(std::string mappedType);
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

  bool operator<(const Sink& other) const;
};

struct Logger {
  std::set<Sink*> sinks;
  std::string filter;
};

struct Logging {
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
