#include "chord.fs.client.options.h"
#include "chord.context.h"

namespace chord {
namespace fs {
namespace client {

void options::clear_source() {
  source.reset();
}

options clear_source(options opt) {
  opt.source = {};
  return opt;
}

options update_source(options opt, const Context& ctxt) {
  opt.source = ctxt.uuid();
  return opt;
}

}
}
}

