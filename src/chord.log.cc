#include "chord.log.h"
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/rotating_file_sink.h>

namespace chord {
namespace log {

void LoggerFactory::setup_chord() {
      if(!chord::file::exists("logs")) {
        chord::file::create_directory("logs");
      }
      const auto rotating_file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>("logs/chord.log", 1024*1024*20, 3);
      rotating_file_sink->set_level(spdlog::level::trace);

      sinks_vector_t chord_sinks;
      chord_sinks.push_back(rotating_file_sink);
      sinks.insert(make_pair(Category::CHORD, chord_sinks));
}

void LoggerFactory::setup_filesystem() {
      const auto stdout_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
      stdout_sink->set_level(spdlog::level::trace);

      sinks_vector_t filesystem_sinks;
      filesystem_sinks.push_back(stdout_sink);
      sinks.insert(make_pair(Category::FILESYSTEM, filesystem_sinks));
    }

LoggerFactory::LoggerFactory() {
      setup_chord();
      setup_filesystem();
}

std::shared_ptr<spdlog::logger> LoggerFactory::get_or_create(std::string name, Category category) {
      auto logger = spdlog::get(name);
      if (!logger) {
        const auto sinks = LoggerFactory::instance().sinks[category];
        logger = std::make_shared<spdlog::logger>(name, begin(sinks), end(sinks));
        logger->set_level(spdlog::level::trace);
        spdlog::register_logger(logger);
      }
      return logger;
    }

std::shared_ptr<spdlog::logger> get_or_create(std::string name, chord::log::Category category) {
  return LoggerFactory::instance().get_or_create(name, category);
}


}  // namespace log
}  // namespace chord
