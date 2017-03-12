#include <chrono>
#include <random>

#include <sstream>

#include "chord.uuid.h"

/**
 * generate random 256-bit number
 */
uuid_t generate_random() {
  unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
  std::default_random_engine generator(seed);
  std::uniform_int_distribution<int64_t> distribution;
  uuid_t _64  = distribution(generator);
  uuid_t _128 = distribution(generator);
  uuid_t _192 = distribution(generator);
  uuid_t _256 = distribution(generator);
  //return (higher << 64 | lower);
  return ( _256 << 192 | _192 << 128 | _128 << 64 | _64 );
}

std::string to_string(const uuid_t& uuid) {
 std::stringstream ss;
 ss << uuid;
 return ss.str();
}
