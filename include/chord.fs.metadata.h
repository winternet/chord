#pragma once
#include <iosfwd>
#include <cstddef>
#include <algorithm>
#include <vector>
#include <iomanip>
#include <set>
#include <string>

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
#include "chord.optional.h"
#include "chord.optional.serialization.h"

namespace boost { namespace serialization { class access; } }

namespace chord {
namespace fs {

struct Metadata {
  friend class boost::serialization::access;

  std::string name;
  std::string owner;
  std::string group;
  perms permissions;
  type file_type;

  // file hash
  chord::optional<chord::uuid> file_hash;

  // reference node
  chord::optional<chord::node> node_ref;

  // replication
  chord::fs::Replication replication;

  /** needed for (de-) serialization **/
  Metadata() = default;
  Metadata(std::string name, std::string owner, std::string group, perms permissions, type file_type, chord::optional<chord::uuid> file_hash={}, chord::optional<chord::node> node_ref={}, chord::fs::Replication replication={})
  : name{name},
    owner{owner},
    group{group},
    permissions{permissions},
    file_type{file_type},
    file_hash{file_hash},
    node_ref{node_ref},
    replication{replication}
    {}

  bool operator<(const Metadata &other) const { return name < other.name; }
  bool operator==(const Metadata &other) const { return name==other.name && file_type==other.file_type; }

  template<class Archive>
  void serialize(Archive & ar, const unsigned int version)
  {
    (void)version;
    ar & name & file_type & owner & group & permissions & node_ref & replication & file_hash;
  }

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
      << std::left << std::setw(static_cast<int>(max_group_len+1)) << metadata.group
      << metadata.name;
    const auto repl = metadata.replication;
    os << " (" << repl.index+1 << '/' << repl.count << ')';
    if(metadata.node_ref) {
      os << " (node_ref: " << *metadata.node_ref << ")";
    }
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

Replication max_replication(const std::set<Metadata>&);
Metadata create_directory();
Metadata create_directory(const std::set<Metadata>&);
bool is_directory(const std::set<Metadata>&);

}  // namespace fs
}  // namespace chord

namespace boost {
namespace serialization {

template<class Archive>
void serialize(Archive & ar, chord::uuid &uuid, const unsigned int version)
{
  (void)version;
  ar & uuid.value();
}

template<class Archive>
void serialize(Archive & ar, chord::node &node, const unsigned int version)
{
  (void)version;
  ar & node.uuid;
  ar & node.endpoint;
}

template<class Archive>
void serialize(Archive & ar, chord::fs::Replication &replication, const unsigned int version)
{
  (void)version;
  ar & replication.index;
  ar & replication.count;
}
}  // namespace serialization
}  // namespace boost
