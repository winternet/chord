#pragma once
#include <thread>
#include <memory>

#include "Fuse.h"
#include "Fuse-impl.h"

#include "chord.peer.h"
#include "chord.utils.h"

namespace chord {
namespace fuse {

class Adapter : public Fusepp::Fuse<Adapter> {
private:
  static constexpr auto logger_name = "chord.fuse.adapter";

  std::shared_ptr<spdlog::logger> logger;
  chord::utils::Options options;
  std::unique_ptr<chord::Peer> peer;
  std::thread peer_thread;

  chord::fs::Facade* filesystem() {
    return this_()->peer->get_filesystem();
  }

  int open_local(const char*, struct fuse_file_info*, mode_t);

public:
  Adapter(int argc, char* arv[]);

  virtual ~Adapter() = default;

  //static chord::Peer* peer();
  static void* init(struct fuse_conn_info*, struct fuse_config*);
  static void destroy(void*);

  static int getattr (const char*, struct stat*, struct fuse_file_info*);
  //TODO implement
  static int setxattr(const char*, const char*, const char*, size_t, int);
  static int utimens(const char *, const struct timespec tv[2], struct fuse_file_info *fi);

  static int readdir(const char*, void*,
                     fuse_fill_dir_t,
                     off_t offset, struct fuse_file_info*,
                     enum fuse_readdir_flags);
  
  static int mknod(const char*, mode_t, dev_t);
  static int mkdir(const char *, mode_t);
  static int create(const char*, mode_t, struct fuse_file_info*);

  static int access(const char *path, int);
  static int open(const char*, struct fuse_file_info*);
  static int release(const char*, struct fuse_file_info*);

  static int read(const char*, char*, size_t size, off_t offset, struct fuse_file_info*);
  static int write(const char*, const char *buf, size_t size, off_t offset, struct fuse_file_info*);
  static int truncate(const char*, off_t offset, struct fuse_file_info*);
  static int flush(const char*, struct fuse_file_info*);

  static int rmdir(const char*);
  static int unlink(const char*);
};

} // namespace fuse
} // namespace chord
