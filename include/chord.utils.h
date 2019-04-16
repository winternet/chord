#pragma once

#include "chord.types.h"

namespace grpc {
  class Status;
}

namespace chord {
namespace utils {

  std::string to_string(const grpc::Status& status);

}  // namespace utils
}  // namespace chord
