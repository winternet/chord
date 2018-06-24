#pragma once

#include <stdexcept>
#include <utility>

#include "chord.optional.h"
#include "chord.grpc.pb.h"

#define throw__exception(message) throw chord::exception{message, __FILE__, __LINE__}
#define throw__grpc_exception(message, status) throw chord::grpc_exception{message, status, __FILE__, __LINE__}

namespace chord {
class exception : public std::runtime_error {
  private:
    std::experimental::optional<std::string> file;
    std::experimental::optional<size_t> line;
    std::string msg;
  public:
    // inherit ctors
    using std::runtime_error::runtime_error;
    explicit exception(const std::string& message, std::string file, size_t line)
        : runtime_error(message), file{file}, line{line}, msg{this->message()} {
        }

    virtual std::string message() const {
      return (file && line)
        ? std::string{runtime_error::what()} + " at " + *file + ":" + std::to_string(*line)
        : std::string{runtime_error::what()};
    }
    const char* what() const noexcept override {
      return msg.c_str();
    }
};

class grpc_exception : public exception {
 private:
  grpc::Status status;
 public:
  explicit grpc_exception(const std::string& message, grpc::Status status)
      : exception(message), status(std::move(status)) {}

  explicit grpc_exception(const std::string& message, grpc::Status status, std::string file, size_t line)
      : exception(message, std::move(file), line), status(std::move(status)) {}

};
}  // namespace chord
