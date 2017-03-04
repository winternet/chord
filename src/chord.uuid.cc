#include "chord.uuid.h"

/**
 * generate random 128-bit number
 */
uuid_t generate_random() {
  unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
  std::default_random_engine generator(seed);
  std::uniform_int_distribution<int64_t> distribution;
  uuid_t lower  = distribution(generator);
  uuid_t higher = distribution(generator);
  return (higher << 64 | lower);
}
