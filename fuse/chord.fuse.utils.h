#pragma once
#include <set>
#include "Fuse.h"
#include "chord.fs.metadata.h"

namespace chord {
namespace fuse {
namespace utils {

  bool set_stat(const std::set<chord::fs::Metadata> metadata, struct stat* stat_buf);

} // namespace utils
} // namespace fuse
} // namespace chord
