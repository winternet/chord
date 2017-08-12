#include <chrono>
#include <random>

#include <sstream>

#include "chord.uuid.h"

using namespace std;

/**
 * generate random 256-bit number
 */
//uuid_t generate_random() {
//  return uuid_t();
//  //unsigned seed = chrono::system_clock::now().time_since_epoch().count();
//  //default_random_engine generator(seed);
//  //uniform_int_distribution<int64_t> distribution;
//  //uuid_t _64  = distribution(generator);
//  //uuid_t _128 = distribution(generator);
//  //uuid_t _192 = distribution(generator);
//  //uuid_t _256 = distribution(generator);
//  //return ( _256 << 192 | _192 << 128 | _128 << 64 | _64 );
//}

//string to_string(const uuid_t& uuid) {
//  stringstream ss;
//  ss << uuid;
//  return ss.str();
//}
