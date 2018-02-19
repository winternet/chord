#pragma once

#include <boost/algorithm/string.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/serialization/set.hpp>
#include <iomanip>
#include <set>

#include "chord.fs.perms.h"
#include "chord.fs.type.h"

namespace chord {
namespace fs {

struct Metadata {
  friend class boost::serialization::access;

  Metadata() = default;
  Metadata(std::string name) : name{std::move(name)} {}
  Metadata(std::string name, std::string owner, std::string group) 
    : name{std::move(name)}, owner{std::move(owner)}, group{std::move(group)} {}
  Metadata(std::string name, std::string owner, std::string group, perms perms) 
    : name{std::move(name)}, owner{std::move(owner)}, group{std::move(group)}, permissions{perms} {}
  Metadata(std::string name, std::set<Metadata> files)
    : name{std::move(name)}, files{std::move(files)} {}

  std::string name;
  std::string owner;
  std::string group;
  perms permissions;
  type file_type;

  //--- only set for directories
  std::set<Metadata> files;

  bool operator<(const Metadata &other) const { return name < other.name; }
  bool operator==(const Metadata &other) const { return name==other.name && files == other.files; }

  template<class Archive>
  void serialize(Archive & ar, const unsigned int version)
  {
    (void)version;
    ar & name & files;
  }

  friend std::ostream &operator<<(std::ostream &os, const Metadata &metadata) {

    std::vector<std::string> owners = {"", metadata.owner};
    std::vector<std::string> groups = {"", metadata.group};
    for(const auto &meta : metadata.files) {
      owners.emplace_back(meta.owner);
      groups.emplace_back(meta.group);
    }

    struct MaxLen {
      bool operator()(const std::string &a, const std::string &b) {
        return a.size() < b.size();
      }

    } cmp;

    int max_owner_len = std::max_element(std::begin(owners), std::end(owners), cmp)->size();
    int max_group_len = std::max_element(std::begin(groups), std::end(groups), cmp)->size();

    os<< metadata.name << ":\n"
      << (metadata.file_type == type::directory ? "d" : "-")
      << metadata.permissions << " "
      << std::left << std::setw(max_owner_len+1) << metadata.owner
      << std::left << std::setw(max_group_len+1) << metadata.group
      << ".\n";


    for(const Metadata& m : metadata.files) {
      os<< (m.file_type == type::directory ? "d" : "-")
        << m.permissions << " " 
        << std::left << std::setw(max_owner_len+1) << m.owner
        << std::left << std::setw(max_group_len+1) << m.group
        << m.name
        << "\n";
    }
    return os;
  }
};

}  // namespace fs
}  // namespace chord
