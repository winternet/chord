#pragma once
#include <exception>
#include <memory>
#include "chord.file.h"
#include "chord.path.h"
#include "chord.log.h"
#include "chord.uuid.h"
#include "chord.test.tmp.file.h"

namespace chord {
namespace test {

struct TmpDir final {

  static constexpr auto logger_name = "chord.test.tmp.dir";

  const chord::path path;
  std::shared_ptr<spdlog::logger> logger;

  TmpDir() : TmpDir(chord::path{chord::uuid::random().string()}) {}

  explicit TmpDir(const std::string& p) : TmpDir(chord::path{p}) {}

  explicit TmpDir(const chord::path& p) : path{p}, logger{log::get_or_create(logger_name)} {
    if(chord::file::exists(path) && !chord::file::is_directory(path)) {
      throw std::runtime_error("Path \'" + path.string() + "\' already exists - aborting.");
    }
    logger->info("creating temporary directory {}", path);
    chord::file::create_directories(path);
  }

  TmpFile add_file() const {
    return TmpFile(path/chord::path{chord::uuid::random().string()});
  }

  TmpDir add_dir() const {
    return TmpDir(path/chord::path{chord::uuid::random().string()});
  }

  TmpFile add_file(const std::string& filename) const {
    return TmpFile(path/filename);
  }

  TmpDir add_dir(const std::string& directory) const {
    return TmpDir(path/directory);
  }

  ~TmpDir() {
    if(!chord::file::exists(path)) return;
    logger->info("removing temporary directory {}", path);
    chord::file::remove_all(path);
  }

  inline operator chord::path() const {
    return path;
  }
};

} // namespace test
} // namespace chord
