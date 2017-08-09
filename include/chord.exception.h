#pragma once

#include <stdexcept>
#include "chord.grpc.pb.h"

using grpc::Status;

namespace chord {
  class exception : public std::runtime_error {
    private:
      Status status;
    public:
      exception(const std::string what, Status status)
        : runtime_error(what), status(status) {}

      exception(const std::string what)
        : runtime_error(what) {}
  };
}
