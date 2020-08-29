#pragma once
#include <exception>
#include <memory>
#include "chord.file.h"
#include "chord.path.h"
#include "chord.log.h"
#include "chord.uuid.h"

namespace chord {
namespace test {

struct TmpBase {

  static constexpr auto logger_name = "chord.test.tmp.base";

  const chord::path path;
  std::shared_ptr<spdlog::logger> logger;

  TmpBase() : TmpBase(chord::path{chord::uuid::random().string()}) {}

  explicit TmpBase(const std::string& p) : TmpBase(chord::path{p}) {}

  explicit TmpBase(const chord::path& p) : path{p}, logger{log::get_or_create(logger_name)} {
  }

  virtual ~TmpBase() {
    if(!chord::file::exists(path)) return;
    logger->info("removing  {}", path);
    chord::file::remove_all(path);
  }

  inline operator chord::path() const {
    return path;
  }
};

} // namespace test
} // namespace chord
