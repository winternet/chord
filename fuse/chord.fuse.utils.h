#pragma once
#include <set>
#include "Fuse.h"
#include "chord.fs.metadata.h"

namespace chord {
namespace fuse {
namespace utils {

  bool set_stat(const chord::fs::Metadata&, struct stat*);
  bool set_stat(const std::set<chord::fs::Metadata>&, struct stat*);
  void set_readdir(const std::set<chord::fs::Metadata>&, fuse_fill_dir_t&, void*);

} // namespace utils
} // namespace fuse
} // namespace chord
