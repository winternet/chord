#pragma once
#include <iosfwd>
#include <cstddef>
#include <algorithm>
#include <vector>
#include <iomanip>
#include <set>
#include <string>
#include <optional>
#include <fmt/ostream.h>
#include <spdlog/fmt/ostr.h>

#include <boost/algorithm/string.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/serialization/map.hpp>
#include <boost/serialization/set.hpp>

#include "chord.uuid.h"
#include "chord.fs.perms.h"
#include "chord.fs.type.h"
#include "chord.node.h"
#include "chord.fs.replication.h"
#include "chord.optional.serialization.h"

namespace boost { namespace serialization { class access; } }
namespace chord { struct Context; }
namespace chord { class uri; }
namespace chord { class path; }

namespace chord {
namespace fs {

struct Metadata {
  friend class boost::serialization::access;

  using size = std::uint64_t;

  std::string name;
  std::string owner;
  std::string group;
  perms permissions;
  type file_type;
  size file_size;

  // file hash
  std::optional<chord::uuid> file_hash;

  // reference node
  std::optional<chord::node> node_ref;

  // replication
  chord::fs::Replication replication;

  /** needed for (de-) serialization **/
  Metadata() = default;
  Metadata(std::string name, std::string owner, std::string group, perms permissions, type file_type, size file_size=0, std::optional<chord::uuid> file_hash={}, std::optional<chord::node> node_ref={}, chord::fs::Replication replication={});

  Metadata(std::string name, std::string owner, std::string group, perms permissions, size file_size, std::optional<chord::uuid> file_hash={}, std::optional<chord::node> node_ref={}, chord::fs::Replication replication={});
  Metadata(std::string name, perms permissions, size file_size, std::optional<chord::uuid> file_hash={}, std::optional<chord::node> node_ref={}, chord::fs::Replication replication={});

  bool operator<(const Metadata &other) const;
  bool operator==(const Metadata &other) const;
  bool compare_basic(const Metadata &other) const;

  static bool equal_basic(const std::set<Metadata>& lhs, const std::set<Metadata>& rhs);

  template<class Archive>
  void serialize(Archive & ar, const unsigned int version);

  friend std::ostream &operator<<(std::ostream &os, const Metadata &metadata) {
    return print(os, metadata, metadata.owner.size(), metadata.group.size());
  }
 
  /**
   * print metadata if max_size of owner and group is known
   */
  friend std::ostream &print(std::ostream& os, const Metadata& metadata, size_t max_owner_len, size_t max_group_len) {
    os<< (metadata.file_type == type::directory ? "d" : "-")
      << metadata.permissions << " "
      << std::left << std::setw(static_cast<int>(max_owner_len+1)) << metadata.owner
      << std::left << std::setw(static_cast<int>(max_group_len+1)) << metadata.group;
    // replication 
    {
      const auto repl = metadata.replication;
      if(repl.count == Replication::ALL.count)
        os << std::left << std::setw(5) << "∞";
      else 
        //os << std::left << std::setw(6) << repl.index+1 << '/' << repl.count;
        os << std::left << std::setw(3) << repl.count;
    }
    // node reference
    {
      if(metadata.node_ref) {
        //os << " (node_ref: " << *metadata.node_ref << ")";
        os << std::left << std::setw(5) << "☇";
      }
    }

    os << metadata.name;
    return os;
  }

  friend std::ostream& operator<<(std::ostream& os, const std::set<Metadata>& metadata) {
    std::vector<std::string> owners;
    std::vector<std::string> groups;

    for(const auto &meta : metadata) {
      owners.emplace_back(meta.owner);
      groups.emplace_back(meta.group);
    }

    struct MaxLen {
      bool operator()(const std::string &a, const std::string &b) {
        return a.size() < b.size();
      }
    } cmp;

    const auto max_owner_len = std::max_element(std::begin(owners), std::end(owners), cmp)->size();
    const auto max_group_len = std::max_element(std::begin(groups), std::end(groups), cmp)->size();
    for(const auto& m:metadata) print(os, m, max_owner_len, max_group_len) << '\n';
    return os;
  }
};

template<class Archive>
void Metadata::serialize(Archive & ar, const unsigned int version)
{
  (void)version;
  ar & name & file_type & file_size & owner & group & permissions & node_ref & replication & file_hash;
}

Replication max_replication(const std::set<Metadata>&);
Metadata create_directory();
Metadata create_directory(const std::string&);
Metadata create_directory(const std::set<Metadata>&);
Metadata create_directory(const std::set<Metadata>&, const std::string&);
bool is_mkdir(const uri&, const std::set<Metadata>&);
bool is_directory(const Metadata&);
bool is_directory(const std::set<Metadata>&);
bool is_empty(const std::set<Metadata>&);
bool is_regular_file(const std::set<Metadata>&);
bool is_shallow_copy(const std::set<Metadata>&, const Context&);
bool is_shallow_copyable(const std::set<Metadata>& metadata, const Context&);
std::set<Metadata> clear_hashes(std::set<Metadata>);
std::set<Metadata> set_node_ref(const std::set<Metadata>& metadata, const Context&);
std::set<Metadata> increase_replication_and_clean(const std::set<Metadata>& metadata);

}  // namespace fs
}  // namespace chord

namespace boost {
namespace serialization {

template<class Archive>
void serialize(Archive & ar, chord::uuid &uuid, [[maybe_unused]] const unsigned int version)
{
  ar & uuid.value();
}

template<class Archive>
void serialize(Archive & ar, chord::node &node, [[maybe_unused]] const unsigned int version)
{
  ar & node.uuid;
  ar & node.endpoint;
}

template<class Archive>
void serialize(Archive & ar, chord::fs::Replication &replication, [[maybe_unused]] const unsigned int version)
{
  ar & replication.index;
  ar & replication.count;
}
}  // namespace serialization
}  // namespace boost

template<> struct fmt::formatter<chord::fs::Metadata> : ostream_formatter {};
