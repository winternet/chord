#include "chord.log.factory.h"

#include "chord.log.h"
#include "chord.path.h"

#include <regex>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/daily_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>

namespace chord {
namespace log {

Factory::Factory(const Logging& logging) : logging{logging} {
}


//void Factory::setup_chord() {
//      if(!chord::file::exists("logs")) {
//        chord::file::create_directory("logs");
//      }
//      const auto rotating_file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>("logs/chord.log", 1024*1024*20, 3);
//      rotating_file_sink->set_level(spdlog::level::trace);
//
//      sinks_vector_t chord_sinks;
//      chord_sinks.push_back(rotating_file_sink);
//      sinks.insert(make_pair(Category::CHORD, chord_sinks));
//}
//
//void Factory::setup_filesystem() {
//      const auto stdout_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
//      stdout_sink->set_level(spdlog::level::trace);
//
//      sinks_vector_t filesystem_sinks;
//      filesystem_sinks.push_back(stdout_sink);
//      sinks.insert(make_pair(Category::FILESYSTEM, filesystem_sinks));
//    }

bool Factory::prepare_file_sink(const std::string& file) const {
  const auto dir = chord::path(file).parent_path();
  if(!chord::file::exists(dir)) {
    return chord::file::create_directories(dir);
  }
  return true;
}

std::shared_ptr<spdlog::sinks::sink> Factory::create_sink(const Sink* sink) const {
  const auto type = sink->type;
  if(type == SinkType::CONSOLE) {
    return std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
  } else if(type == SinkType::FILE) {
    prepare_file_sink(sink->path);
    return std::make_shared<spdlog::sinks::basic_file_sink_mt>(sink->path);
  } else if(type == SinkType::FILE_ROTATING) {
    prepare_file_sink(sink->path);
    return std::make_shared<spdlog::sinks::rotating_file_sink_mt>(sink->path, /* max size*/ 1024*1024*50, /* max_files */ 5);
  } else if(type == SinkType::FILE_DAILY) {
    prepare_file_sink(sink->path);
    return std::make_shared<spdlog::sinks::daily_file_sink_mt>(sink->path, /* hour */ 0, /* minute */0);
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
