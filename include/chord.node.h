#pragma once

#include <iostream>

#include "chord.types.h"
#include "chord.uuid.h"
#include "chord.optional.h"

namespace chord {
struct node {
  // can not be const since type must be 
  // moveable for optional<node>
  uuid_t uuid;
  chord::endpoint endpoint;

  bool operator==(const node& other) const {
    return uuid == other.uuid 
           && endpoint == other.endpoint;
  }

  bool operator!=(const node& other) const {
    return !(*this == other);
  }

  bool operator<(const node& other) const {
    return uuid < other.uuid
      && endpoint < other.endpoint;
  }

  std::string string() const {
    std::stringstream ss;
    ss << *this;
    return ss.str();
  }

  friend std::ostream& operator<<(std::ostream& os, const node& n) {
    return os << n.uuid << "@" << n.endpoint;
  }

  friend std::ostream& operator<<(std::ostream& os, const optional<node>& n) {
    if(n) return os << n->uuid << "@" << n->endpoint;
    return os << "<unknown>";
  }
};
}  // namespace chord

