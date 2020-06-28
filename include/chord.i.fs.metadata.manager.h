#pragma once

#include <set>
#include <map>

#include "chord.fs.metadata.h"
#include "chord.uri.h"

namespace chord {
namespace fs {

class IMetadataManager {

protected:

  struct uri_descending {
    bool operator()(const chord::uri& lhs, const chord::uri& rhs) const {
      const auto lhs_str = lhs.path().canonical().string();
      const auto rhs_str = rhs.path().canonical().string();
      const auto lhs_cnt = std::count(lhs_str.begin(), lhs_str.end(), '/');
      const auto rhs_cnt = std::count(rhs_str.begin(), rhs_str.end(), '/');
      return lhs_cnt != rhs_cnt ? lhs_cnt > rhs_cnt : lhs > rhs;
    }
  };

 public:
  using uri_meta_map_desc = std::map<chord::uri, std::set<Metadata>, uri_descending>;

  virtual ~IMetadataManager() = default;

  virtual std::set<Metadata> del(const chord::uri& directory) = 0;
  virtual std::set<Metadata> del(const chord::uri& directory, const std::set<Metadata> &metadata, const bool removeIfEmpty=false) = 0;

  virtual std::set<Metadata> dir(const chord::uri& directory) = 0;

  virtual bool add(const chord::uri& directory, const std::set<Metadata>& metadata) = 0;

  virtual uri_meta_map_desc get_all() = 0;
  virtual uri_meta_map_desc get(const chord::uuid& from, const chord::uuid& to) = 0;

  virtual bool exists(const chord::uri& uri) = 0;

  virtual uri_meta_map_desc get_shallow_copies(const chord::node& node) = 0;
  virtual uri_meta_map_desc get_replicated(const std::uint32_t min_idx=0) = 0;
  virtual std::set<Metadata> get(const chord::uri& directory) = 0;

};

} //namespace fs
} //namespace chord
