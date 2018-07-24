#pragma once

#include <functional>
#include <vector>

namespace chord {

/*
 * @brief produces a set of chord::uuids
 * @todo  let the callback return an iterator, however, the metadata might
 *        change during the runtime of the call...
 */
using TakeProducerCallback = std::function< std::vector<chord::TakeResponse>(const chord::uuid&, const chord::uuid&) >;
using take_producer_t = TakeProducerCallback;


using TakeConsumerCallback = std::function< void(const chord::TakeResponse&) >;
using take_consumer_t = TakeConsumerCallback;
}  // namespace chord
