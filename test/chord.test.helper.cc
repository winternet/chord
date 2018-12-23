#include "chord.test.helper.h"
#include "chord.context.h"
#include "chord.uuid.h"
#include "chord_common.pb.h"

namespace chord {
namespace test {
namespace helper {

using chord::common::Header;
using chord::common::RouterEntry;

Context make_context(const uuid &self) {
  Context context = Context();
  context.set_uuid(self);
  return context;
}

RouterEntry make_entry(const uuid &id, const endpoint_t &addr) {
  RouterEntry entry;
  entry.set_uuid(id);
  entry.set_endpoint(addr);
  return entry;
}

Header make_header(const uuid &id, const endpoint_t &addr) {
  Header header;
  header.mutable_src()->CopyFrom(make_entry(id, addr));
  return header;
}

Header make_header(const uuid &id, const char* addr) {
  return make_header(id, endpoint_t{addr});
}

} //namespace helper
} //namespace test
} //namespace chord
