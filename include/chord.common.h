#pragma once

#include "chord.pb.h"
#include "chord.context.h"

namespace chord {
  namespace common {

    chord::common::Header make_header(const Context& context);

  }
}
