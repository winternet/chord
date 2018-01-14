#pragma once

#include <boost/algorithm/string.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/serialization/set.hpp>
#include <set>

#include "chord.fs.perms.h"
#include "chord.fs.type.h"

namespace chord {
namespace fs {

struct Metadata {
  friend class boost::serialization::access;

  Metadata() = default;
  Metadata(std::string name) : name{name} {}
  Metadata(std::string name, std::set<Metadata> files)
    : name{name}, files{files} {}

  std::string name;
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
};

}  // namespace fs
}  // namespace chord
