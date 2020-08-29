#pragma once
#include <memory>
#include <exception>
#include <string_view>

#include "chord.file.h"
#include "chord.path.h"
#include "chord.log.h"
#include "chord.uuid.h"
#include "chord.test.tmp.file.h"
#include "chord.test.tmp.base.h"

namespace chord {
namespace test {

struct TmpDir final : public TmpBase {

private:
  static constexpr auto logger_name = "chord.test.tmp.dir";
  std::shared_ptr<spdlog::logger> logger;

  mutable std::vector<std::shared_ptr<TmpBase>> contents;

public:
  TmpDir() : TmpDir(chord::path{chord::uuid::random().string()}) {}

  explicit TmpDir(const std::string& p) : TmpDir(chord::path{p}) {}
  explicit TmpDir(const chord::path& p) : TmpBase(p), logger{log::get_or_create(logger_name)} {
    if(chord::file::exists(path) && !chord::file::is_directory(path)) {
      throw std::runtime_error("Path \'" + path.string() + "\' already exists - aborting.");
    }
    logger->info("creating temporary directory {}", path);
    chord::file::create_directories(path);
  }

  std::shared_ptr<TmpFile> add_file() const {
    auto file = std::make_shared<TmpFile>(path/chord::path{chord::uuid::random().string()});
    contents.push_back(file);
    return file;
  }

  std::shared_ptr<TmpDir> add_dir() const {
    auto dir = std::make_shared<TmpDir>(path/chord::path{chord::uuid::random().string()});
    contents.push_back(dir);
    return dir;
  }

  std::shared_ptr<TmpFile> add_file(const std::string_view filename) const {
    const auto file = std::make_shared<TmpFile>(path/filename);
    contents.push_back(file);
    return file;
  }

  std::shared_ptr<TmpDir> add_dir(const std::string_view directory) const {
    auto dir = std::make_shared<TmpDir>(path/directory);
    contents.push_back(dir);
    return dir;
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
