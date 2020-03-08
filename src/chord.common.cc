#include "chord.common.h"

#include "chord_common.pb.h"

#include "chord.context.h"
#include "chord.node.h"

using chord::common::Header;
using chord::common::RouterEntry;

namespace chord {
namespace common {

chord::common::Header make_header(const chord::node &node) {
  Header header;
  header.mutable_src()->CopyFrom(make_entry(node));
  return header;
}

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

RouterEntry make_entry(const chord::node& node) {
  RouterEntry ret;
  ret.set_uuid(node.uuid);
  ret.set_endpoint(node.endpoint);
  return ret;
}

}
}
