#pragma once

#include "chord.file.h"
#include "chord.uri.h"
#include "chord.context.h"
#include "chord.fs.monitor.h"

namespace chord {
namespace fs {
namespace util {

bool remove(const chord::path&, chord::fs::monitor* monitor=nullptr);
bool remove(const chord::uri&, const Context&, chord::fs::monitor*, bool delete_empty=true);
bool remove(const chord::uri&, const Context&, bool delete_empty=true);

path as_journal_path(const Context&, const path&);
path as_journal_path(const Context&, const uri&);

} // namespace util
} // namespace fs
} // namespace chord
