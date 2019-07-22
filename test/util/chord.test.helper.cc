#include "chord.test.helper.h"
#include "chord.context.h"
#include "chord.uuid.h"
#include "chord.node.h"
#include "chord_common.pb.h"
#include "chord.types.h"
#include "chord.test.tmp.dir.h"

namespace chord {
namespace test {

using chord::test::TmpDir;
using chord::common::Header;
using chord::common::RouterEntry;

Context make_context(const uuid &self, const TmpDir& data_directory) {
  Context context = Context();
  context.set_uuid(self);
  context.data_directory = data_directory;
  return context;
}

Context make_context(const uuid &self, const TmpDir& data_directory, const TmpDir& meta_directory) {
  Context context = Context();
  context.set_uuid(self);
  context.data_directory = data_directory;
  context.meta_directory = meta_directory;
  return context;
}

Context make_context(const uuid &self) {
  Context context = Context();
  context.set_uuid(self);
  return context;
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

Header make_header(const uuid &id, const endpoint &addr) {
  Header header;
  header.mutable_src()->CopyFrom(make_entry(id, addr));
  return header;
}

Header make_header(const uuid &id, const char* addr) {
  return make_header(id, endpoint{addr});
}

} //namespace test
} //namespace chord