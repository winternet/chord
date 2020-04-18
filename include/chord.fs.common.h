#pragma once

#include <grpcpp/impl/codegen/status.h>
#include <grpcpp/impl/codegen/status_code_enum.h>

namespace chord {
namespace fs {

bool is_successful(const ::grpc::Status& status);

}
}
