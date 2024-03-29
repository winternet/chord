#include "chord.log.h"

#include <utility>
#include <vector>

#include "chord.exception.h"
#include "chord.log.factory.h"

namespace chord {
namespace log {

bool Formatter::operator==(const Formatter& other) const {
  return pattern == other.pattern;
}

bool Formatter::operator<(const Formatter& other) const {
  return pattern < other.pattern;
}

SinkType SinkType::CONSOLE("console");
SinkType SinkType::FILE_ROTATING("file-rotating");
SinkType SinkType::FILE_DAILY("file-daily");
SinkType SinkType::FILE("file");

std::set<SinkType>& SinkType::types() {
  static std::set<SinkType> sinkTypes;
  return sinkTypes;
}

SinkType::SinkType(std::string mappedType) noexcept
  : mappedType{std::move(mappedType)} {
  types().insert(*this);
}

SinkType SinkType::from(std::string str) {
  for(const auto& val:types()) {
    if(val.mappedType == str) {
      return val;
    }
  }
  throw__exception("unknown type: \'"+str+"\'");
}

bool SinkType::operator==(const SinkType& other) const {
  return other.mappedType == mappedType;
}

bool SinkType::operator!=(const SinkType& other) const {
  return !(*this == other);
}

bool SinkType::operator<(const SinkType& other) const {
  return mappedType < other.mappedType;
}


/** Sink **/
bool Sink::operator<(const Sink& other) const {
  return (type < other.type && path < other.path && *formatter < *other.formatter);
}

std::shared_ptr<spdlog::logger> get_or_create(std::string name) {
  static std::mutex mtx;
  std::unique_lock<std::mutex> lck(mtx);
  auto logger = spdlog::get(name);
  if(!logger) {
    logger = spdlog::stdout_color_mt(name);
  }
  return logger;
}

Factory Logging::factory() const {
  return Factory(*this);
}

}  // namespace log
}  // namespace chord
