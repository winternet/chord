#pragma once

#include <string>
#include <vector>
#include <map>
#include <set>

#include "chord.exception.h"

namespace chord {

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
};

}
