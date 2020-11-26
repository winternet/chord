#include "chord.fuse.adapter.h"

#include <set>
#include <string>
#include <iostream>

#include "Fuse-impl.h"
#include "chord.utils.h"
#include "chord.fs.metadata.h"
#include "chord.fuse.utils.h"

namespace chord {
namespace fuse {

static const std::string root_path = "/";
static const std::string hello_str = "Hello World!\n";
static const std::string hello_path = "/hello";


Adapter::Adapter(int argc, char* argv[]): argc{argc}, argv{argv} {
  logger = chord::log::get_or_create(logger_name);
  options = chord::utils::parse_program_options(argc, argv);
  peer = std::make_unique<chord::Peer>(options.context);
}


void* Adapter::init([[maybe_unused]]struct fuse_conn_info *conn, [[maybe_unused]]struct fuse_config *cfg) {
  this_()->peer_thread = std::thread(&chord::Peer::start, this_()->peer.get());
  return this_();
}

void Adapter::destroy([[maybe_unused]]void* private_data) {
  this_()->peer.reset();
  this_()->peer_thread.join();
}

int Adapter::getattr(const char *path, [[maybe_unused]]struct stat *stbuf, [[maybe_unused]]struct fuse_file_info *)
{
	int res = 0;
	memset(stbuf, 0, sizeof(struct stat));

  std::set<fs::Metadata> metadata;
  const auto status = this_()->filesystem()->dir(chord::utils::as_uri(path), metadata);
  const auto succ = utils::set_stat(metadata, stbuf);

  if(!status.ok() || !succ) {
    return -ENOENT;
  }

  return res;
}

int Adapter::readdir(const char *path, void *buf, fuse_fill_dir_t filler,
			               off_t, struct fuse_file_info *, enum fuse_readdir_flags)
{
  std::set<fs::Metadata> metadata;
  const auto status = this_()->filesystem()->dir(chord::utils::as_uri(path), metadata);

  if(!status.ok()) {
    return -ENOENT;
  }

  utils::set_readdir(metadata, filler, buf);

	return 0;
}


int Adapter::open(const char *path, struct fuse_file_info *fi)
{
  //TODO
  //  get file if path is a directory
	if (path != hello_path) {
		return -ENOENT;
  }

	if ((fi->flags & 3) != O_RDONLY) {
		return -EACCES;
  }

	return 0;
}


int Adapter::read(const char *path, char *buf, size_t size, off_t offset,
		              struct fuse_file_info *)
{
	if (path != hello_path) {
		return -ENOENT;
  }

	size_t len;
	len = hello_str.length();
	if ((size_t)offset < len) {
		if (offset + size > len)
			size = len - offset;
		memcpy(buf, hello_str.c_str() + offset, size);
	} else
		size = 0;

	return size;
}

} // namespace fuse
} // namespace chord
