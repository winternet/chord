#pragma once

#include <set>
#include <map>

#include "chord.fs.metadata.h"
#include "chord.uri.h"

namespace chord {
namespace fs {

class IMetadataManager {

 public:
  virtual ~IMetadataManager() = default;

  virtual std::set<Metadata> del(const chord::uri& directory) = 0;
  virtual std::set<Metadata> del(const chord::uri& directory, const std::set<Metadata> &metadata, const bool removeIfEmpty=false) = 0;

  virtual std::set<Metadata> dir(const chord::uri& directory) = 0;

  virtual bool add(const chord::uri& directory, const std::set<Metadata>& metadata) = 0;

  virtual std::map<chord::uri, std::set<Metadata> > get_all() = 0;
  virtual std::map<chord::uri, std::set<Metadata> > get(const chord::uuid& from, const chord::uuid& to) = 0;

  virtual bool exists(const chord::uri& uri) = 0;

  virtual std::map<chord::uri, std::set<Metadata>> get_shallow_copies(const chord::node& node) = 0;
  virtual std::map<chord::uri, std::set<Metadata>> get_replicated() = 0;
  virtual std::set<Metadata> get(const chord::uri& directory) = 0;

};

} //namespace fs
} //namespace chord
