#include <string>
#include <grpcpp/impl/codegen/status.h>

namespace chord {
namespace utils {

  std::string to_string(const grpc::Status& status) {
    return status.error_message() + " ("+status.error_details()+")";
  }

}  // namespace utils
}  // namespace chord
