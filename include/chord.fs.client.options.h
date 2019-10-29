#pragma once

#include "chord.fs.replication.h"
#include "chord.uuid.h"

namespace chord {
namespace fs {
namespace client {
  struct options {
    Replication replication = Replication::NONE;
    chord::uuid source = chord::uuid::random();
    union {
      bool recursive; // del
      bool rebalance; // put
    };
  };
}
}
}
