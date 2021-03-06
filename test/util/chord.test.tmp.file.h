#pragma once
#include <exception>
#include <fstream>
#include <memory>
#include "chord.file.h"
#include "chord.path.h"
#include "chord.log.h"
#include "chord.uuid.h"

#include "chord.test.tmp.base.h"

namespace chord {
namespace test {

struct TmpFile final : public TmpBase {

  static constexpr auto logger_name = "chord.test.tmp.file";
  std::shared_ptr<spdlog::logger> logger;

  TmpFile() : TmpFile(chord::path{chord::uuid::random().string()}) {}

  TmpFile(const chord::path& p) : TmpBase(p), logger{log::get_or_create(logger_name)} {
    if(chord::file::exists(path)) {
      throw std::runtime_error("Path \'" + path.string() + "\' already exists - aborting.");
    }
    if(!path.parent_path().empty() && !chord::file::exists(path.parent_path())) {
      throw std::runtime_error("Parent path of \'"+path.string()+"\' does not exist - aborting.");
    }
    logger->info("creating temporary file {}", path);
    std::ofstream file(path);
    file << chord::uuid::random().string();
    file.close();
  }

  ~TmpFile() {
    remove();
  }

  void remove() const {
    if(!chord::file::exists(path)) return;

    logger->info("removing temporary file {}", path);
    chord::file::remove(path);
  }

  inline operator chord::path() const {
    return path;
  }
};

} // namespace test
} // namespace chord
