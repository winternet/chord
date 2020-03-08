#pragma once

#include "chord.fs.replication.h"
#include "chord.uuid.h"
#include "chord.context.h"
#include "chord.optional.h"

namespace chord {
namespace fs {
namespace client {
  struct options {
    Replication replication = Replication::NONE;
    chord::optional<chord::uuid> source;
    union {
      bool recursive; // del
      bool rebalance; // put
    };

    void clear_source();
  };
  options clear_source(options);
  options update_source(options, const Context&);
}
}
}
