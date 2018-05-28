#pragma once

#include <spdlog/spdlog.h>
#include <spdlog/fmt/bundled/ostream.h>

namespace chord {
namespace log {

// NOTE this might be slow since get and create locks the registry
//     with a mutex
std::shared_ptr<spdlog::logger> get_or_create(std::string name);

}  // namespace log
}  // namespace chord
