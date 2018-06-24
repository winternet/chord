#pragma once

#include <experimental/optional>

namespace chord {

  template<class T>
  using optional = std::experimental::optional<T>;

} // namespace chord

