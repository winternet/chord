#pragma once
#include <exception>
#include <memory>
#include "chord.file.h"
#include "chord.path.h"
#include "chord.log.h"
#include "chord.uuid.h"

namespace chord {
namespace test {

struct TmpDir final {

  static constexpr auto logger_name = "chord.test.tmp.dir";

  const chord::path path;
  std::shared_ptr<spdlog::logger> logger;

  TmpDir() : TmpDir(chord::path{chord::uuid::random().string()}) {}

  TmpDir(const chord::path& p) : path{p}, logger{log::get_or_create(logger_name)} {
    if(chord::file::exists(path)) {
      throw std::runtime_error("Path \'" + path.string() + "\' already exists - aborting.");
    }
    logger->info("creating temporary directory {}.", path);
    chord::file::create_directories(path);
  }

  ~TmpDir() {
    logger->info("removing temporary directory {}.", path);
    chord::file::remove_all(path);
  }

  inline operator chord::path() const {
    return path;
  }
};

} // namespace test
} // namespace chord
