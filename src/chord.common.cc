#include "chord.context.h"
#include "chord.grpc.pb.h"

using chord::common::Header;
using chord::common::RouterEntry;

namespace chord {
namespace common {

chord::common::Header make_header(const Context &context) {
  Header header;
  RouterEntry src;
  src.set_uuid(context.uuid());
  src.set_endpoint(context.bind_addr);
  header.mutable_src()->CopyFrom(src);
  return header;
}

chord::node make_node(const RouterEntry& entry) {
  return {uuid_t{entry.uuid()}, entry.endpoint()};
}

}
}
