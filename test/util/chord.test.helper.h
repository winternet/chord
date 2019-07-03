#pragma once
#include "chord.types.h"

namespace chord { class uuid; }
namespace chord { struct Context; }
namespace chord { struct node; }
namespace chord { namespace common { class RouterEntry; } }
namespace chord { namespace common { class Header; } }
namespace chord { namespace test { class TmpDir; } }

namespace chord {
namespace test {

Context make_context(const uuid &self, const chord::test::TmpDir& data_directory, const chord::test::TmpDir& meta_directory);
Context make_context(const uuid &self, const chord::test::TmpDir& data_directory);
Context make_context(const uuid &self);

chord::common::RouterEntry make_entry(const chord::node& n);
chord::common::RouterEntry make_entry(const uuid &id, const chord::endpoint& addr);

chord::common::Header make_header(const uuid &id, const chord::endpoint &addr);

chord::common::Header make_header(const uuid &id, const char* addr);

} // namespace test
} // namespace chord
