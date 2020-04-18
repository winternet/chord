#include "chord.fs.common.h"

namespace chord {
namespace fs {

bool is_successful(const ::grpc::Status& status) {
  return status.ok() || status.error_code() == ::grpc::StatusCode::ALREADY_EXISTS;
}

} //namespace fs
} //namespace chord
