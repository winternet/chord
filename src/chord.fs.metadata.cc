#include "chord.fs.metadata.h"

using std::begin;
using std::end;

namespace chord {
namespace fs {

Metadata::Metadata(std::string name, std::string owner, std::string group, perms permissions, type file_type, chord::optional<chord::uuid> file_hash, chord::optional<chord::node> node_ref, chord::fs::Replication replication)
  : name{name},
    owner{owner},
    group{group},
    permissions{permissions},
    file_type{file_type},
    file_hash{file_hash},
    node_ref{node_ref},
    replication{replication}
    {}

bool Metadata::operator<(const Metadata &other) const { 
  return name < other.name; 
}
bool Metadata::operator==(const Metadata &other) const { 
  return name == other.name 
    && file_type == other.file_type 
    //&& owner == other.owner 
    //&& group == other.group
    && node_ref == other.node_ref
    && replication == other.replication;
    //&& file_hash == other.file_hash; 
}

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
  return std::any_of(begin(metadata), end(metadata), [&](const Metadata& m) { return m.name == "." && m.file_type == type::directory; });
}

bool is_shallow_copy(const std::set<Metadata>& metadata) {
  return is_regular_file(metadata) && metadata.begin()->node_ref;
}

bool is_regular_file(const std::set<Metadata>& metadata) {
  return metadata.size() == 1 && metadata.begin()->file_type == type::regular;
}

std::set<Metadata> clear_hashes(std::set<Metadata> metadata) {
  std::set<Metadata> retVal;
  std::transform(begin(metadata), end(metadata), std::inserter(retVal, begin(retVal)), [](auto m) {
      m.file_hash.reset();
      return m;
  });
  return retVal;
}


} // namespace fs
} // namespace chord
