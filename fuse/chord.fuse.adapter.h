#pragma once
#include <thread>
#include <memory>

#include "Fuse.h"
#include "Fuse-impl.h"

#include "chord.peer.h"

namespace chord {
namespace fuse {

class Adapter : public Fusepp::Fuse<Adapter> {
private:
  static constexpr auto logger_name = "chord.fuse.adapter";

  std::thread peer_thread;
  std::unique_ptr<chord::Peer> peer;
  std::shared_ptr<spdlog::logger> logger;

  chord::fs::Facade* filesystem() {
    return peer->get_filesystem();
  }

public:
  Adapter(int argc, char* arv[]);

  virtual ~Adapter();

  //static chord::Peer* peer();
  static void* init(struct fuse_conn_info *conn, struct fuse_config *cfg);
  static void destroy(void*);

  static int getattr (const char *, struct stat *, struct fuse_file_info *);

  static int readdir(const char *path, void *buf,
                     fuse_fill_dir_t filler,
                     off_t offset, struct fuse_file_info *fi,
                     enum fuse_readdir_flags);
  
  static int open(const char *path, struct fuse_file_info *fi);

  static int read(const char *path, char *buf, size_t size, off_t offset,
                  struct fuse_file_info *fi);
};

} // namespace fuse
} // namespace chord
