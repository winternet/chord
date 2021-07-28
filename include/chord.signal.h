#pragma once

#include <sigslot/signal.hpp>

namespace chord {

template<typename... T>
using signal = sigslot::signal<T...>;

} // namespace chord
