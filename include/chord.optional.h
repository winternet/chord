#pragma once

#include <experimental/optional>

namespace chord {

  template<class T>
  using optional = std::experimental::optional<T>;

  using nullopt_t = std::experimental::nullopt_t;

} // namespace chord

