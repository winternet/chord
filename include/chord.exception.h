#pragma once

#include <stdexcept>
#include <utility>
#include "chord.grpc.pb.h"

namespace chord {
class exception : public std::runtime_error {
 private:
  grpc::Status status;
 public:
  exception(const std::string& what, grpc::Status status)
      : runtime_error(what), status(std::move(status)) {}

  explicit exception(const std::string& what)
      : runtime_error(what) {}
};
}
