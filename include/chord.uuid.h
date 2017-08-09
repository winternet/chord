#pragma once
#include <chrono>
#include <random>

#include <boost/multiprecision/cpp_int.hpp>

namespace chord {
  class uuid {
    private:
      typedef boost::multiprecision::cpp_int value_t;

      value_t val;
    public:
      uuid() {};
      uuid(const std::string& str) {val = value_t{str};}
      template<typename T>
      uuid(const T& v) { val = value_t{v}; }
      template<typename T>
      uuid(const T beg, const T end) {
        boost::multiprecision::import_bits(val, (unsigned int*)beg, (unsigned int*)end);
      }

      value_t& value() { return val; }

      static const int UUID_MAX = 255;

      //todo(christoph) implement tests
      static uuid random() {
        auto array = std::array<char, UUID_MAX>{};
        const int seed = std::chrono::system_clock::now().time_since_epoch().count();

        std::default_random_engine generator(seed);
        std::uniform_int_distribution<char> distribution;

        std::generate(std::begin(array), std::end(array), std::bind(distribution, generator));

        return uuid{ std::begin(array), std::end(array) };
      }

      uuid operator+=(const uuid& other) const { return uuid{val+other.val}; }
      bool operator==(const uuid& other) const { return val == other.val; }
      bool operator>=(const uuid& other) const { return val >= other.val; }
      bool operator<=(const uuid& other) const { return val <= other.val; }
      bool operator< (const uuid& other) const { return val <  other.val; }
      bool operator> (const uuid& other) const { return val >  other.val; }

      friend std::istream& operator>>(std::istream& is, uuid& hash) {
        is >> hash.val;
        return is;
      }
      friend std::ostream& operator<<(std::ostream& os, const uuid& hash) {
        os << hash.val;
        return os;
      }
  };
};

typedef chord::uuid uuid_t;
std::string to_string(const uuid_t& uuid);
