#pragma once
#include "chord.types.h"

namespace chord {

class uuid;
class node;
class Context;

namespace common{
class RouterEntry;
class Header;
}

namespace test {

class TmpDir;

namespace helper {

Context make_context(const uuid &self, const TmpDir& data_directory, const TmpDir& meta_directory);
Context make_context(const uuid &self, const TmpDir& data_directory);
Context make_context(const uuid &self);

chord::common::RouterEntry make_entry(const chord::node& n);
chord::common::RouterEntry make_entry(const uuid &id, const chord::endpoint& addr);

chord::common::Header make_header(const uuid &id, const chord::endpoint &addr);

chord::common::Header make_header(const uuid &id, const char* addr);

} // namespace helper
} // namespace test
} // namespace chord
