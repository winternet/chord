#include "chord.fs.client.options.h"

namespace chord {
namespace fs {
namespace client {

  options clear_source(const options& options) {
    auto o = options;
    o.source.reset();
    return o;
  }

  options init_source(const options& options, const Context& context) {
    if(options.source) return options;

    auto o = options;
    o.source = context.uuid();
    return o;
  }
} // namespace client
} // namespace fs
} // namespace chord
