#include "chord.fuse.utils.h"


namespace chord {
namespace fuse {
namespace utils {

  bool set_stat(const std::set<chord::fs::Metadata> metadata, struct stat* stbuf) {
    if(metadata.size() != 1) return false;
    const auto m = metadata.cbegin();

    if(chord::fs::is_directory(metadata)) {
      stbuf->st_mode = S_IFDIR | 0755;
      stbuf->st_nlink = metadata.size();
    } else if(chord::fs::is_regular_file(metadata)) {
      stbuf->st_mode = S_IFREG | 0444;
      stbuf->st_nlink = 1;
      stbuf->st_size = m->file_size;
    }
    return true;
  }

} // namespace utils
} // namespace fuse
} // namespace chord
