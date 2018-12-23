
namespace chord {

class uuid;
class endpoint;
class Context;

namespace common{
class RouterEntry;
class Header;
}

namespace test {
namespace helper {

Context make_context(const uuid &self);

chord::common::RouterEntry make_entry(const uuid &id, const endpoint & addr);

chord::common::Header make_header(const uuid &id, const endpoint &addr);

chord::common::Header make_header(const uuid &id, const char* addr);

} // namespace helper
} // namespace test
} // namespace chord
