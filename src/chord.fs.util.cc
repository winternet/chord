#include "chord.fs.util.h"

namespace chord {
namespace fs {
namespace util {

bool remove(const chord::path& path, chord::fs::monitor* monitor) {
  //if(!chord::file::exists(path)) return false;

  if(monitor) {
    auto lock = chord::fs::monitor::lock(monitor, {path});
    return chord::file::remove(path);
  } else {
    return chord::file::remove(path);
  }
}
bool remove(const chord::uri& uri, const Context& context, chord::fs::monitor* monitor, bool delete_empty) {
  auto path = context.data_directory / uri.path();

  auto removed = remove(path, monitor);
  for(auto parent=path.parent_path(); delete_empty && parent > context.data_directory && chord::file::exists(parent) && chord::file::is_empty(parent); parent=parent.parent_path()) {
    removed |= remove(parent, monitor);
  }

  return removed;
}

bool remove(const chord::uri& uri, const Context& context, bool delete_empty) {
  return remove(uri, context, nullptr, delete_empty);
}

path as_journal_path(const Context& context, const path& path) {
  const auto parent_path = context.journal_directory() / path.parent_path();
  if(!file::exists(parent_path)) {
    file::create_directories(parent_path);
  }
  return parent_path / path.filename();
}

path as_journal_path(const Context& context, const uri& uri) {
  return as_journal_path(context, uri.path());
}

} // namespace util
} // namespace fs
} // namespace chord
