#pragma once

#include <optional>
#include "chord.fs.replication.h"
#include "chord.uuid.h"
#include "chord.uri.h"
#include "chord.context.h"

namespace chord {
namespace fs {
namespace client {
  struct options {
    Replication replication = Replication::NONE;
    std::optional<chord::uuid> source;
    union {
      bool recursive = false; // del
      bool rebalance; // put
    };
    bool force; // mov / put

    options() = default;
    explicit options(const Replication& replication) : replication{replication} {}


    void clear_source();
  };

  options clear_source(const options&);
  options init_source(const options&, const Context&);

} // namespace client
} // namespace fs
} // namespace chord
