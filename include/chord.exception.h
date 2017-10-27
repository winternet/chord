#pragma once

#include <stdexcept>
#include "chord.grpc.pb.h"

namespace chord {
  class exception : public std::runtime_error {
    private:
      grpc::Status status;
    public:
      exception(const std::string what, grpc::Status status)
        : runtime_error(what), status(status) {}

      exception(const std::string what)
        : runtime_error(what) {}
  };
}
