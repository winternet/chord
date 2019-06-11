#include "chord.log.factory.h"

#include <map>
#include <set>
#include <vector>
#include <regex>

#include "chord.file.h"
#include "chord.log.h"
#include "chord.path.h"

#include <spdlog/spdlog.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/daily_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>

namespace chord {
namespace log {

bool Factory::valid_level(const std::string level) const {
  const spdlog::level::level_enum l = spdlog::level::from_str(logging.level);
  return spdlog::level::to_string_view(l) == level;
}

Factory::Factory(const Logging& logging) : logging{logging} {
  if(!logging.level.empty()) {
    if(!valid_level(logging.level)) {
      spdlog::error("Failed to parse root log level: {}", logging.level);
    } else {
      spdlog::set_level(spdlog::level::from_str(logging.level));
    }
  }
}

bool Factory::prepare_file_sink(const std::string& file) const {
  const auto dir = chord::path(file).parent_path();
  if(!chord::file::exists(dir)) {
    return chord::file::create_directories(dir);
  }
  return true;
}

template<class T>
std::shared_ptr<T> Factory::configure_sink(std::shared_ptr<T> sink, const Sink* sink_conf) const {
  if(!sink_conf->level.empty()) {
    sink->set_level(spdlog::level::from_str(sink_conf->level));
  }
  return sink;
}

std::shared_ptr<spdlog::sinks::sink> Factory::create_sink(const Sink* sink_conf) const {
  const auto type = sink_conf->type;
  if(type == SinkType::CONSOLE) {
    return configure_sink(std::make_shared<spdlog::sinks::stdout_color_sink_mt>(), sink_conf);
  } else if(type == SinkType::FILE) {
    prepare_file_sink(sink_conf->path);
    return configure_sink(std::make_shared<spdlog::sinks::basic_file_sink_mt>(sink_conf->path), sink_conf);
  } else if(type == SinkType::FILE_ROTATING) {
    prepare_file_sink(sink_conf->path);
    return configure_sink(std::make_shared<spdlog::sinks::rotating_file_sink_mt>(sink_conf->path, /* max size*/ 1024*1024*50, /* max_files */ 5), sink_conf);
  } else if(type == SinkType::FILE_DAILY) {
    prepare_file_sink(sink_conf->path);
    return configure_sink(std::make_shared<spdlog::sinks::daily_file_sink_mt>(sink_conf->path, /* hour */ 0, /* minute */0), sink_conf);
  }

  return std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
}

std::shared_ptr<spdlog::logger> Factory::create_logger(const std::string& name, const Logger& logger) const {
  std::vector<spdlog::sink_ptr> sinks;
  for(const auto* sink_ptr : logger.sinks) {
    if(sink_ptr) sinks.push_back(create_sink(sink_ptr));
  }
  return std::make_shared<spdlog::logger>(name, begin(sinks), end(sinks));
}

std::shared_ptr<spdlog::logger> Factory::get_or_create(std::string name) {
  auto logger = spdlog::get(name);
  if(logger) return logger;

  for(const auto& [k,log_conf] : logging.loggers) {
    std::regex filter_regex(log_conf.filter);
    if(std::regex_search(name, filter_regex)) {
      logger = create_logger(name, log_conf);
      if(!log_conf.level.empty()) {
        logger->set_level(spdlog::level::from_str(log_conf.level));
      }
      spdlog::register_logger(logger);
    }
  }

  if(!logger) {
    spdlog::warn("Couldn't find logger configuration for \'{}\', falling back.", name);
    logger = spdlog::stdout_color_mt(name);
  }
  return logger;
}


}  // namespace log
}  // namespace chord
