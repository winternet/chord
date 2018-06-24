#pragma once

#include <iostream>

#include "chord.types.h"
#include "chord.uuid.h"
#include "chord.optional.h"

namespace chord {
struct node {
  const uuid_t uuid;
  const endpoint_t endpoint;

  friend std::ostream& operator<<(std::ostream& os, const node& n) {
    return os << n.uuid << "@" << n.endpoint;
  }
  friend std::ostream& operator<<(std::ostream& os, const optional<node>& n) {
    if(n) return os << n->uuid << "@" << n->endpoint;
    return os << "<unknown>";
  }
};
}  // namespace chord
