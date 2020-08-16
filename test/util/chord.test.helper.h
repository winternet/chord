#pragma once
#include <memory>
#include <thread>

#include "../chord.peer.int.h"
#include "chord.types.h"

namespace chord { class uuid; }
namespace chord { struct Context; }
namespace chord { struct node; }
namespace chord { namespace common { class RouterEntry; } }
namespace chord { namespace common { class Header; } }
namespace chord { namespace test { class TmpDir; } }

namespace chord {
namespace test {

chord::IntPeer* make_peer(const Context&);

Context make_context(const uuid self, const endpoint bind_addr, const TmpDir& data_directory, const TmpDir& meta_directory, const endpoint join_addr, const bool bootstrap);
Context make_context(const uuid self, const endpoint bind_addr, const TmpDir& data_directory, const TmpDir& meta_directory);
Context make_context(const uuid self, const chord::test::TmpDir& data_directory, const chord::test::TmpDir& meta_directory);
Context make_context(const uuid self, const chord::test::TmpDir& data_directory);
Context make_context(const uuid self);

std::thread detatch(chord::Peer* peer, bool wait=true);

chord::common::RouterEntry make_entry(const chord::node& n);
chord::common::RouterEntry make_entry(const uuid &id, const chord::endpoint& addr);

chord::common::Header make_header(const uuid &id, const chord::endpoint &addr);

chord::common::Header make_header(const uuid &id, const char* addr);

} // namespace test
} // namespace chord
