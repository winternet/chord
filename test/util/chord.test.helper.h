#pragma once
#include <memory>
#include <thread>

#include "../chord.peer.int.h"
#include "chord.types.h"
#include "chord.test.tmp.dir.h"

namespace chord { class uuid; }
namespace chord { struct Context; }
namespace chord { struct node; }
namespace chord { namespace common { class RouterEntry; } }
namespace chord { namespace common { class Header; } }
namespace chord { namespace test { class TmpDir; } }

namespace chord {
namespace test {

struct IntContext {
  static const std::string bind_addr;

  int index;
  std::shared_ptr<TmpDir> data;
  std::shared_ptr<TmpDir> meta;

  Context context;

  IntContext(int, TmpDir&);
};

chord::IntPeer* make_peer(const Context&);

Context make_context(const uuid self, const endpoint bind_addr, const std::shared_ptr<TmpDir> data_directory, const std::shared_ptr<TmpDir> meta_directory, const endpoint join_addr, const bool bootstrap);
Context make_context(const uuid self, const endpoint bind_addr, const std::shared_ptr<TmpDir> data_directory, const std::shared_ptr<TmpDir> meta_directory);
Context make_context(const uuid self, const std::shared_ptr<TmpDir> data_directory, const std::shared_ptr<TmpDir> meta_directory);
Context make_context(const uuid self, const std::shared_ptr<TmpDir> data_directory);
Context make_context(const uuid self);

std::thread detatch(chord::Peer* peer, bool wait=true);

chord::common::RouterEntry make_entry(const chord::node& n);
chord::common::RouterEntry make_entry(const uuid &id, const chord::endpoint& addr);

chord::common::Header make_header(const uuid &id, const chord::endpoint &addr);

chord::common::Header make_header(const uuid &id, const char* addr);

} // namespace test
} // namespace chord
