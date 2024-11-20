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

#ifdef CHORD_GCOV_FLUSH
extern "C" void __gcov_dump();
#endif

namespace chord {
namespace fuse {

Adapter::Adapter(int argc, char* argv[]): 
  logger{chord::log::get_or_create(logger_name)},
  options{chord::utils::parse_program_options(argc, argv)},
  peer{std::make_unique<chord::Peer>(options.context)} {}

Adapter::~Adapter() {
#ifdef CHORD_GCOV_FLUSH
  ::__gcov_dump();
#endif
}

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
  auto fs = this_()->filesystem();
  auto logger = this_()->logger;

  const auto uri = chord::utils::as_uri(path);
  const auto journal_path = util::as_journal_path(this_()->options.context, uri);

  //TODO condense this logic and make transactional
  if(this_()->open_local(journal_path.string().c_str(), fi, mode)) {
    logger->error("[create] failed to open file locally {}", journal_path);
    return -EAGAIN;
  }

  const auto status = fs->put(journal_path, uri);
  if(!status.ok()) {
    logger->error("failed to put newly created file {} under {}", journal_path, uri);
    return -EAGAIN;
  }
  return 0;
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
  return Adapter::create(path, mode, nullptr);
}

int Adapter::access(const char *path, int mask) {
  const auto uri = chord::utils::as_uri(path);
  auto fs = this_()->filesystem();
  const auto status =  fs->exists(uri).error_code();
  switch(status) {
    case grpc::StatusCode::ALREADY_EXISTS:
    case grpc::StatusCode::OK:
      return 0;
    case grpc::StatusCode::NOT_FOUND:
      return -ENOENT;
    default:
      return -EACCES; // permission denied
  }
}

bool Adapter::clean_journal(const char* path) const {
  const auto uri = chord::utils::as_uri(path);
  return clean_journal(uri);
}

bool Adapter::clean_journal(const chord::uri& uri) const {
  const auto journal_path = chord::fs::util::as_journal_path(this_()->options.context, uri);
  return chord::file::remove_all(journal_path);
}

int Adapter::rename(const char* from, const char* to, unsigned int flags) {
  auto logger = this_()->logger;
  auto fs = this_()->filesystem();

  const auto force = !(flags & RENAME_NOREPLACE);

  logger->info("rename {} -> {}", from, to);
  const auto src = chord::utils::as_uri(from);
  const auto dst = chord::utils::as_uri(to);

  const auto status = fs->move(src, dst, force);

  if(status.ok()) {
    this_()->clean_journal(src);
    this_()->clean_journal(dst);
  }

  switch(status.error_code()) {
    case grpc::StatusCode::ALREADY_EXISTS:
      return -EEXIST;
    case grpc::StatusCode::NOT_FOUND:
      return -ENOENT;
    case grpc::StatusCode::OK:
      return 0;
    default:
      return -EAGAIN;
  }
}

int Adapter::mkdir(const char *path, [[maybe_unused]] mode_t mode) {
  const auto uri = chord::utils::as_uri(path);
  const auto status = this_()->filesystem()->mkdir(uri);
  return status.error_code();
}

int Adapter::open_local(const char* path, struct fuse_file_info* fi, mode_t mode) {
  auto logger = this_()->logger;
  logger->info("opening file {}", path);
  const int fd = ::open(path, fi->flags, mode);
  if(fd < 0) {
    logger->error("failed to open file {}", std::strerror(errno));
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

  // NOTE: we re-use files from journal, in order to operate on the latest version all other operations
  // like delete and move have to assert that they clear the cache accordingly
  if(!chord::file::exists(journal_path)) {
    const auto status = this_()->filesystem()->get(uri, journal_path);
    if(!status.ok()) {
      //TODO: handle status code correctly
      return -ENOENT;
    }
  }

  return this_()->open_local(journal_path.string().c_str(), fi, 0);
}

int Adapter::release(const char *path, struct fuse_file_info *fi) {
  this_()->logger->info("release {}", path);
  //FIXME: called once for each file, replace with put
  const auto status = Adapter::flush(path, fi);

  //TODO error handling
  if(!status) return -ENOENT;

  // cleanup journal
  this_()->clean_journal(path);

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
}

int Adapter::flush(const char *path, struct fuse_file_info* fi) {
  auto logger = this_()->logger;
  auto fs = this_()->filesystem();
  logger->info("flush {}", path);

  const auto uri = chord::utils::as_uri(path);
  const auto journal_path = chord::fs::util::as_journal_path(this_()->options.context, uri);
  const auto data_path = this_()->options.context.data_directory / chord::path{path};

  const auto succ = ::fsync(fi->fh);
  logger->info("flush {}, file_size {}", path, chord::file::file_size(journal_path));

  const auto status = fs->put(journal_path, uri);
  if(!status.ok()) {
    logger->error("failed to put {} to {}", journal_path, uri);
    return -EAGAIN;
  }
  return succ;
}

int Adapter::rmdir(const char* path) {
  return Adapter::unlink(path);
}

int Adapter::unlink(const char* path) {
  auto fs = this_()->filesystem();
  auto logger = this_()->logger;

  const auto uri = chord::utils::as_uri(path);
  const auto status = fs->del(uri);
  if(!status.ok()) return -ENOENT;

  this_()->clean_journal(uri);

  return 0;
}

} // namespace fuse
} // namespace chord
