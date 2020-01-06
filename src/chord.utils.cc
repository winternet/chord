#include "chord.utils.h"

#include <grpcpp/impl/codegen/status.h>

namespace chord {
namespace utils {

  std::string to_string(const grpc::Status& status) {
    return status.error_details().empty() ? status.error_message() :
      status.error_message() + " ("+status.error_details()+")";
  }

}  // namespace utils
}  // namespace chord
