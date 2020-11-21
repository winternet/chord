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


Adapter::Adapter(int argc, char* argv[]) {
  // note that argv contains arguments for fuse, might not work at all
  const auto options = chord::utils::parse_program_options(argc, argv);
  const auto context = options.context;

  logger = chord::log::get_or_create(logger_name);

  peer = std::make_unique<chord::Peer>(options.context);
}

Adapter::~Adapter() {
  logger->info("~Adapter");
}

void Adapter::destroy([[maybe_unused]]void* private_data) {
   this_()->peer.reset();
   this_()->peer_thread.join();
  //if(Adapter* adapter = reinterpret_cast<Adapter*>(private_data)) {
  //  adapter->logger->info("[destroy] resetting peer, joining thread.");
  //  adapter->peer.reset();
  //  adapter->peer_thread.join();
  //}
}

void* Adapter::init([[maybe_unused]]struct fuse_conn_info *conn, [[maybe_unused]]struct fuse_config *cfg) {
  //logger->info("init");
  auto* peer = this_()->peer.get();
  this_()->peer_thread = std::thread(&chord::Peer::start, peer);
  return this_();
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


	if (path == root_path) {
		stbuf->st_mode = S_IFDIR | 0755;
		stbuf->st_nlink = 2;
	} else if (path == hello_path) {
		stbuf->st_mode = S_IFREG | 0444;
		stbuf->st_nlink = 1;
		stbuf->st_size = hello_str.length();
	} else
		res = -ENOENT;

	return res;
}

int Adapter::readdir(const char *path, void *buf, fuse_fill_dir_t filler,
			               off_t, struct fuse_file_info *, enum fuse_readdir_flags)
{
	if (path != root_path)
		return -ENOENT;

	filler(buf, ".", NULL, 0, FUSE_FILL_DIR_PLUS);
	filler(buf, "..", NULL, 0, FUSE_FILL_DIR_PLUS);
	filler(buf, hello_path.c_str() + 1, NULL, 0, FUSE_FILL_DIR_PLUS);

	return 0;
}


int Adapter::open(const char *path, struct fuse_file_info *fi)
{
	if (path != hello_path)
		return -ENOENT;

	if ((fi->flags & 3) != O_RDONLY)
		return -EACCES;

	return 0;
}


int Adapter::read(const char *path, char *buf, size_t size, off_t offset,
		              struct fuse_file_info *)
{
	if (path != hello_path)
		return -ENOENT;

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
