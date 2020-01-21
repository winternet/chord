#include "chord.common.h"

#include "chord_common.pb.h"

#include "chord.context.h"
#include "chord.node.h"

using chord::common::Header;
using chord::common::RouterEntry;

namespace chord {
namespace common {

Header make_header(const chord::node &node) {
  Header header;
  RouterEntry src;
  src.set_uuid(node.uuid);
  src.set_endpoint(node.endpoint);
  header.mutable_src()->CopyFrom(src);
  return header;
}

Header make_header(const Context &context) {
  Header header;
  RouterEntry src;
  src.set_uuid(context.uuid());
  src.set_endpoint(context.bind_addr);
  header.mutable_src()->CopyFrom(src);
  return header;
}

node make_node(const RouterEntry& entry) {
  return {uuid_t{entry.uuid()}, entry.endpoint()};
}

RouterEntry make_entry(const node& n) {
  return make_entry(n.uuid, n.endpoint);
}

RouterEntry make_entry(const uuid &id, const endpoint& addr) {
  RouterEntry entry;
  entry.set_uuid(id);
  entry.set_endpoint(addr);
  return entry;
}

}
}
