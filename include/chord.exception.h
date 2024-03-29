#pragma once

#include <stdexcept>
#include <utility>
#include <optional>

#include "chord.grpc.pb.h"

#define throw__exception(message) throw chord::exception{message, __FILE__, __LINE__}
#define throw__fs_exception(message) throw chord::fs_exception{message, __FILE__, __LINE__}
#define throw__grpc_exception(status) throw chord::grpc_exception{status, __FILE__, __LINE__}

namespace chord {
class exception : public std::runtime_error {
  private:
    std::optional<std::string> file;
    std::optional<size_t> line;
    std::string msg;
  public:
    // inherit ctors
    using std::runtime_error::runtime_error;
    explicit exception(const std::string& message, std::string t_file, size_t t_line)
        : runtime_error(message), file{t_file}, line{t_line}, msg{message} {
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
  explicit grpc_exception(const grpc::Status& status)
      : exception(status.error_message()), status(std::move(status)) {}

  explicit grpc_exception(const grpc::Status& t_status, std::string t_file, size_t t_line)
      : exception{t_status.error_message(), std::move(t_file), t_line}, status{std::move(t_status)} {}

};

class fs_exception : public exception {
  public:
    explicit fs_exception(const std::string& message, std::string t_file, size_t t_line)
        : chord::exception(message, t_file, t_line) {
        }
};
}  // namespace chord
