#include "chord.fuse.adapter.h"

#include <set>
#include <string>
#include <iostream>

#include "Fuse-impl.h"
#include "chord.utils.h"
#include "chord.fs.util.h"
#include "chord.fs.metadata.h"
#include "chord.fuse.utils.h"

using namespace chord::fs;

namespace chord {
namespace fuse {

static const std::string root_path = "/";
static const std::string hello_str = "Hello World!\n";
static const std::string hello_path = "/hello";


Adapter::Adapter(int argc, char* argv[]): 
  logger{chord::log::get_or_create(logger_name)},
  options{chord::utils::parse_program_options(argc, argv)},
  peer{std::make_unique<chord::Peer>(options.context)} {}


void* Adapter::init([[maybe_unused]]struct fuse_conn_info *conn, [[maybe_unused]]struct fuse_config *cfg) {
  this_()->peer_thread = std::thread(&chord::Peer::start, this_()->peer.get());
  return this_();
}

void Adapter::destroy([[maybe_unused]]void* private_data) {
  this_()->peer.reset();
  this_()->peer_thread.join();
}

int Adapter::getattr(const char *path, [[maybe_unused]]struct stat *stbuf, [[maybe_unused]]struct fuse_file_info *) {
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
			               off_t, struct fuse_file_info *, enum fuse_readdir_flags) {
  std::set<fs::Metadata> metadata;
  const auto status = this_()->filesystem()->dir(chord::utils::as_uri(path), metadata);

  if(!status.ok()) {
    return -ENOENT;
  }

  utils::set_readdir(metadata, filler, buf);

	return 0;
}

int Adapter::create(const char* path, mode_t mode, struct fuse_file_info* fi) {
  // only handle files
  if(!(mode & S_IFREG)) {
    return -ENOSYS;
  }
  
  auto fs = this_()->filesystem();

  const auto uri = chord::utils::as_uri(path);
  const auto journal_path = util::as_journal_path(this_()->options.context, uri);

  //TODO move "exist" logic to fs.facade
  std::set<fs::Metadata> metadata;
  const auto status = fs->dir(uri, metadata);
  if(status.error_code() != grpc::StatusCode::NOT_FOUND) {
    return -EEXIST;
  }

  if(chord::file::exists(journal_path)) {
    chord::file::remove(journal_path);
  }

  if(!chord::file::create_directories(journal_path.parent_path()) && !chord::file::create_file(journal_path)) {
    return -ENXIO;
  }

  return this_()->open_local(journal_path.string().c_str(), fi);
}

//TODO implement
int Adapter::setxattr (const char*, const char*, const char*, size_t, int) {
  return 0;
}
//TODO implement
int Adapter::utimens(const char* path, const struct timespec tv[2], struct fuse_file_info *fi) {
  return 0;
}

int Adapter::mknod(const char *path, mode_t mode, [[maybe_unused]]dev_t dev) {
  return Adapter::create(path, (mode_t)(mode | S_IFREG), nullptr);
}

int Adapter::open_local(const char* path, struct fuse_file_info* fi) {
  const int fd = ::open(path, fi->flags);
  if(fd < 0) {
    return -1;
  }

  fi->fh = fd;
  return 0;
}

int Adapter::open(const char* path, struct fuse_file_info *fi) {
  this_()->logger->info("open {}", path);
  //TODO
  //  get file if path is a directory
  const auto uri = chord::utils::as_uri(path);
  const auto journal_path = chord::fs::util::as_journal_path(this_()->options.context, uri);

  if(!chord::file::exists(journal_path)) {
    const auto status = this_()->filesystem()->get(uri, journal_path);
    if(!status.ok()) {
      //TODO: handle status code correctly
      return -ENOENT;
    }
  }

  return this_()->open_local(journal_path.string().c_str(), fi);
}

int Adapter::release(const char *path, struct fuse_file_info *fi) {
  this_()->logger->info("release {}", path);
  //FIXME: called once for each file, replace with put
  const auto succ = Adapter::flush(path, fi);
  if(succ) {
    //TODO cleanup recursively
    const auto journal_path = chord::fs::util::as_journal_path(this_()->options.context, chord::path{path});
    chord::file::remove(journal_path);
  }
  //TODO error handling
  return 0;
}

int Adapter::truncate(const char *path, off_t offset, struct fuse_file_info* fi) {
  this_()->logger->info("truncate {}", path);
  const auto succ = ::ftruncate(fi->fh, offset);
  const auto journal_path = chord::fs::util::as_journal_path(this_()->options.context, chord::path{path});
  this_()->logger->info("truncate {}, file_size {}", path, chord::file::file_size(journal_path));
  return succ;
}

int Adapter::write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
  this_()->logger->info("write {}", path);
  const auto succ =  ::pwrite(fi->fh, buf, size, offset);
  const auto journal_path = chord::fs::util::as_journal_path(this_()->options.context, chord::path{path});
  this_()->logger->info("write {}, file_size {}", path, chord::file::file_size(journal_path));
  return succ;
}

int Adapter::read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
  this_()->logger->info("read {}", path);
  return ::pread(fi->fh, buf, size, offset);
  //const auto journal_path = chord::fs::util::as_journal_path(this_()->options.context, chord::path{path});

  //if(!chord::file::exists(journal_path)) {
  //  //TODO handle issue when open obviously went wrong or file has been deleted meanwhile...
  //  return -ENOENT;
  //}

	//size_t len;
	//len = hello_str.length();
	//if ((size_t)offset < len) {
	//	if (offset + size > len)
	//		size = len - offset;
	//	memcpy(buf, hello_str.c_str() + offset, size);
	//} else
	//	size = 0;

	//return size;
}

int Adapter::flush(const char *path, struct fuse_file_info* fi) {
  this_()->logger->info("flush {}", path);
  const auto uri = chord::utils::as_uri(path);
  const auto journal_path = chord::fs::util::as_journal_path(this_()->options.context, uri);

  const auto succ = ::fsync(fi->fh);
  this_()->logger->info("flush {}, file_size {}", path, chord::file::file_size(journal_path));

  chord::file::copy_file(journal_path, this_()->options.context.data_directory / chord::path{path}, true);

  const auto status = this_()->filesystem()->put(journal_path, uri);
  if(!status.ok()) {
    return -EAGAIN;
  }
  return succ;
}

} // namespace fuse
} // namespace chord
