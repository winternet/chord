#include "chord.log.h"

namespace chord {
namespace log {

// NOTE this might be slow since get and create locks the registry
//     with a mutex
std::shared_ptr<spdlog::logger> get_or_create(std::string name) {
  auto logger = spdlog::get(name);
  if (!logger) {
    logger = spdlog::stdout_logger_mt(name);
  }
  return logger;
}
}  // namespace log
}  // namespace chord
