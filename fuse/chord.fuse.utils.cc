#include "chord.fuse.utils.h"


namespace chord {
namespace fuse {
namespace utils {

bool set_stat(const chord::fs::Metadata& metadata, struct stat* stbuf) {
  switch(metadata.file_type) {
    case fs::type::regular:
      stbuf->st_mode = S_IFREG | 0444;
      stbuf->st_nlink = 1;
      stbuf->st_size = metadata.file_size;
      break;
    case fs::type::directory:
      stbuf->st_mode = S_IFDIR | 0755;
      stbuf->st_nlink = 1;
      break;
    default:
      return false;
  }

  return true;
}

bool set_stat(const std::set<chord::fs::Metadata>& metadata, struct stat* stbuf) {

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


void set_readdir(const std::set<chord::fs::Metadata>& metadata, fuse_fill_dir_t& filler, void* buf) {
  for(const auto& m : metadata) {
    filler(buf, m.name.c_str(), nullptr, 0, FUSE_FILL_DIR_PLUS);
  }
	//filler(buf, ".", NULL, 0, FUSE_FILL_DIR_PLUS);
	//filler(buf, "..", NULL, 0, FUSE_FILL_DIR_PLUS);
	//filler(buf, hello_path.c_str() + 1, NULL, 0, FUSE_FILL_DIR_PLUS);
}

} // namespace utils
} // namespace fuse
} // namespace chord
