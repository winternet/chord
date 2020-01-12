#pragma once
#include "chord.router.h"

namespace chord {
class RouterSpy : public Router {
  public:
    explicit RouterSpy(chord::Context &context) : Router(context) {};

    virtual ~RouterSpy() = default;

    RouterEntry entry(const size_t idx) const {
      auto it = successors.begin();
      std::advance(it, idx);
      return *it;
    }

};
}  // namespace chord
