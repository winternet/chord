#pragma once

#include "chord.file.h"
#include "chord.singleton.h"

#include <map>
#include <vector>

#include <spdlog/spdlog.h>
#include <spdlog/fmt/bundled/ostream.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

namespace chord {
namespace log {

enum class Category {
  CHORD,
  FILESYSTEM
};

class LoggerFactory final : public Singleton<LoggerFactory> {
  public:
    using sinks_vector_t = std::vector<spdlog::sink_ptr>;
    using sinks_map_t = std::map<Category, sinks_vector_t>;
  private:
    sinks_map_t sinks;

    void setup_chord();

    void setup_filesystem();

  public:
    LoggerFactory();

    std::shared_ptr<spdlog::logger> get_or_create(std::string name, Category category);
};

// NOTE this might be slow since get and create locks the registry
//     with a mutex
std::shared_ptr<spdlog::logger> get_or_create(std::string name, Category category=Category::CHORD);


}  // namespace log
}  // namespace chord
