#include "chord.fs.metadata.h"

using std::begin;
using std::end;

namespace chord {
namespace fs {

Metadata create_directory() {
  Metadata meta;
  meta.name = ".";
  meta.permissions = perms::none;
  meta.file_type = type::directory;
  return meta;
}

Replication max_replication(const std::set<Metadata>& metadata) {
  return std::max_element(begin(metadata), end(metadata), [&](const Metadata& lhs, const Metadata& rhs) {
      return lhs.replication < rhs.replication;
  })->replication;
}

Replication max_replication_index_zero(const std::set<Metadata>& metadata) {
  const auto repl = max_replication(metadata);
  if(repl) {
    return Replication(0, repl.count);
  }
  return repl;
}

Metadata create_directory(const std::set<Metadata>& metadata) {
  auto meta = create_directory();
  meta.replication = max_replication_index_zero(metadata);
  return meta;
}

bool is_directory(const std::set<Metadata>& metadata) {
  return std::any_of(begin(metadata), end(metadata), [&](const Metadata& m) { return m.file_type == type::directory; });
}

} // namespace fs
} // namespace chord
