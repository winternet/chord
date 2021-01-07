#include "chord.fs.metadata.h"
#include "chord.context.h"
#include "chord.utils.h"
#include "chord.uri.h"
#include "chord.path.h"

using std::begin;
using std::end;

namespace chord {
namespace fs {

Metadata::Metadata(std::string name, std::string owner, std::string group, perms permissions, type file_type, size file_size, std::optional<chord::uuid> file_hash, std::optional<chord::node> node_ref, chord::fs::Replication replication)
  : name{name},
    owner{owner},
    group{group},
    permissions{permissions},
    file_type{file_type},
    file_size{file_size},
    file_hash{file_hash},
    node_ref{node_ref},
    replication{replication}
    {}

Metadata::Metadata(std::string name, perms permissions, size file_size, std::optional<chord::uuid> file_hash, std::optional<chord::node> node_ref, chord::fs::Replication replication)
  : Metadata(name, "", "", permissions, type::regular, file_size, file_hash, node_ref, replication) {};

Metadata::Metadata(std::string name, std::string owner, std::string group, perms permissions, size file_size, std::optional<chord::uuid> file_hash, std::optional<chord::node> node_ref, chord::fs::Replication replication)
  : Metadata(name, owner, group, permissions, type::regular, file_size, file_hash, node_ref, replication) {};
//name{name},
//    permissions{permissions},
//    file_type{file_type},
//    file_size{file_size},
//    file_hash{file_hash},
//    node_ref{node_ref},
//    replication{replication}
//    {}

bool Metadata::operator<(const Metadata &other) const { 
  return name < other.name; 
}

bool Metadata::equal_basic(const std::set<Metadata>& lhs, const std::set<Metadata>& rhs) {
    return std::equal(lhs.begin(), lhs.end(), rhs.begin(), rhs.end(), [&](const auto& lhs, const auto& rhs) {return lhs.compare_basic(rhs);});
}

bool Metadata::compare_basic(const Metadata &other) const {
  return name == other.name 
    && file_type == other.file_type
    && file_hash == other.file_hash;
    //&& owner == other.owner 
    //&& group == other.group
    //&& node_ref == other.node_ref
    //&& replication == other.replication;
    //&& file_hash == other.file_hash; 
}

bool Metadata::operator==(const Metadata &other) const { 
  return compare_basic(other)
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

std::set<Metadata> create_directory(const std::set<Metadata>& metadata, const std::string& folder_name) {
  auto meta_dir = create_directory(metadata);
  meta_dir.name = folder_name;
  return {meta_dir};
}

bool is_directory(const Metadata& metadata) {
  return metadata.name == "." && metadata.file_type == type::directory;
}

bool is_mkdir(const uri& uri, const std::set<Metadata>& metadata) {
  return metadata.size() == 1 
    && metadata.begin()->file_type == type::directory 
    && metadata.begin()->name == uri.path().filename();
}

bool is_directory(const std::set<Metadata>& metadata) {
  return std::any_of(begin(metadata), end(metadata), [&](const Metadata& m) { return is_directory(m); });
}

bool is_empty(const std::set<Metadata>& metadata) {
  return metadata.size() == 1 && is_directory(metadata);
}

bool is_shallow_copy(const std::set<Metadata>& metadata, const Context& context) {
  return is_regular_file(metadata) 
    && metadata.begin()->node_ref && *metadata.begin()->node_ref != context.node();
}

bool is_shallow_copyable(const std::set<Metadata>& metadata, const Context& context) {
  return is_regular_file(metadata) 
    && max_replication(metadata) == Replication::NONE
    && (!metadata.begin()->node_ref || *metadata.begin()->node_ref != context.node());
}

bool is_regular_file(const std::set<Metadata>& metadata) {
  return metadata.size() == 1 && metadata.begin()->file_type == type::regular;
}

std::set<Metadata> set_node_ref(const std::set<Metadata>& metadata, const Context& context) {
  std::set<Metadata> ret;
  std::transform(metadata.begin(), metadata.end(), std::inserter(ret, ret.begin()), [&](auto meta) { 
      meta.node_ref = context.node();
      return meta;
  });
  return ret;
}

std::set<Metadata> clear_hashes(std::set<Metadata> metadata) {
  std::set<Metadata> retVal;
  std::transform(begin(metadata), end(metadata), std::inserter(retVal, begin(retVal)), [](auto m) {
      m.file_hash.reset();
      return m;
  });
  return retVal;
}

std::set<Metadata> increase_replication_and_clean(const std::set<Metadata>& metadata) {
  std::set<Metadata> next_repl;
  std::transform(metadata.begin(), metadata.end(), std::inserter(next_repl, next_repl.begin()), [](Metadata m) { ++m.replication; return m; });
  chord::utils::erase_if(next_repl, [](const Metadata& m) {return !m.replication;});
  return next_repl;
}


} // namespace fs
} // namespace chord
