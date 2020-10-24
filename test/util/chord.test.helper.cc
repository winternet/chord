#include <chrono>

#include "chord.test.helper.h"
#include "chord.context.h"
#include "chord.uuid.h"
#include "chord.node.h"
#include "chord.path.h"
#include "chord_common.pb.h"
#include "chord.types.h"
#include "chord.test.tmp.dir.h"
#include "../chord.peer.int.h"

namespace chord {
namespace test {

using chord::test::TmpDir;
using chord::common::Header;
using chord::common::RouterEntry;

const std::string IntContext::bind_addr = "127.0.0.1";

IntContext::IntContext(int idx, TmpDir& base_dir) :
  index{idx},
  data{base_dir.add_dir("data" + std::to_string(idx))},
  meta{base_dir.add_dir("meta" + std::to_string(idx))}
{
  const auto port = 50050+idx;
  context = test::make_context({"0"}, {bind_addr+":"+std::to_string(port)}, data, meta);
}


chord::IntPeer* make_peer(const Context& context) {
  return new chord::IntPeer(context);
  //return std::make_shared<chord::IntPeer>(context);
}

Context make_context(const uuid self, const std::shared_ptr<TmpDir> data_directory) {
  Context context = Context();
  context.set_uuid(self);
  context.data_directory = data_directory->path;
  return context;
}

Context make_context(const uuid self, const std::shared_ptr<TmpDir> data_directory, const std::shared_ptr<TmpDir> meta_directory) {
  return make_context(self, endpoint(), data_directory, meta_directory);
}

Context make_context(const uuid self, const endpoint bind_addr, const std::shared_ptr<TmpDir> data_directory, const std::shared_ptr<TmpDir> meta_directory) {
  return make_context(self, bind_addr, data_directory, meta_directory, endpoint(), true);
}

Context make_context(const uuid self, const endpoint bind_addr, const std::shared_ptr<TmpDir> data_directory, const std::shared_ptr<TmpDir> meta_directory, const endpoint join_addr, const bool bootstrap) {
  Context context = Context();
  context.set_uuid(self);
  context.data_directory = data_directory->path;
  context.meta_directory = meta_directory->path;
  context.bind_addr = bind_addr;
  context.advertise_addr = bind_addr;
  context.join_addr = join_addr;
  context.bootstrap = bootstrap;
  return context;
}

std::thread start_thread(chord::Peer* peer, bool wait) {
  using namespace std::chrono_literals;
  std::thread t(&chord::Peer::start, peer);
  if(wait) std::this_thread::sleep_for(100ms);
  return t;
}

Context make_context(const uuid self) {
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
