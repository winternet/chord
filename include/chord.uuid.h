#pragma once

#include <boost/multiprecision/cpp_int.hpp>
#include <chrono>
#include <limits>
#include <random>
#include <sstream>
#include <string>

namespace chord {
class uuid {
public:
  using value_t = boost::multiprecision::uint256_t;
 private:

  static constexpr int UUID_BITS_MAX = 256;

  value_t val;

 public:
  uuid() = default;

  explicit inline uuid(const std::string &str) : val{value_t{str}} {}

  template <typename T>
  //cppcheck-suppress noExplicitConstructor
  inline uuid(const T &v) : val{value_t{v}} {}

  template <typename Iterator>
  inline uuid(const Iterator beg, const Iterator end) {
    boost::multiprecision::import_bits(val,
                                       reinterpret_cast<const unsigned int *>(beg),
                                       reinterpret_cast<const unsigned int *>(end));
  }

  inline value_t &value() { return val; }


  /**
   * generate random 256-bit number
   */
  static uuid random() {
    auto array = std::array<char, UUID_BITS_MAX / 8 / sizeof(char)>{};
    const auto seed =
        std::chrono::system_clock::now().time_since_epoch().count();

    std::default_random_engine generator(seed);
    std::uniform_int_distribution<char> distribution;

    std::generate(std::begin(array), std::end(array),
                  std::bind(distribution, generator));

    return uuid{std::begin(array), std::end(array)};
  }

  /**
   * implicit string conversion operator
   */
  inline operator std::string() const { return string(); }

  /**
   * value as string
   */
  inline std::string string() const {
    return val.str();
  }

  /**
   * value as hex
   */
  inline std::string hex() const {
    std::stringstream ss;
    ss << std::hex << val;
    auto str = ss.str();
    std::transform(str.begin(), str.end(), str.begin(), ::tolower);
    return str;
  }

  inline std::string short_hex() const { return hex().substr(0, 5); }

  inline uuid &operator+=(const uuid &other) {
    val += other.val;
    return *this;
  }

  inline uuid &operator-=(const uuid &other) {
    val -= other.val;
    return *this;
  }

  inline uuid &operator*=(const uuid &other) {
    val *= other.val;
    return *this;
  }

  inline uuid &operator/=(const uuid &other) {
    val /= other.val;
    return *this;
  }

  inline uuid &operator%=(const uuid &other) { 
    val %= other.val;
    return *this;
  }

  /**
   * @brief Check if the uuid is in the interval between the two given uuids on the ring.
   *
   * Neither of the boundaries is included in the interval. If both uuids
   * match the interval is assumed to span the whole uuid ring.
   */
  inline bool between(const uuid& from, const uuid& to) const {
    if(from == to) {
      return (*this != from);
    }

    // interval does not cross zero
    if(from < to) {
      return (from < *this && *this < to);
    }

    const value_t max_value = std::numeric_limits<value_t>::max();
    const value_t min_value = std::numeric_limits<value_t>::min();

    // first interval (from, max]
    // second interval [min, to)
    return (from != max_value && from < *this && *this <= max_value)
      || (to != min_value && *this < to && *this >= min_value);
  }

  inline bool operator==(const uuid &other) const { return val == other.val; }

  inline bool operator!=(const uuid &other) const { return val != other.val; }

  inline bool operator>=(const uuid &other) const { return val >= other.val; }

  inline bool operator<=(const uuid &other) const { return val <= other.val; }

  inline bool operator<(const uuid &other) const { return val < other.val; }

  inline bool operator>(const uuid &other) const { return val > other.val; }

  inline uuid operator+(const uuid &other) const { return uuid{val + other.val}; }

  inline uuid operator-(const uuid &other) const { return uuid{val - other.val}; }

  inline uuid operator*(const uuid &other) const { return uuid{val * other.val}; }

  inline uuid operator/(const uuid &other) const { return uuid{val / other.val}; }

  inline uuid operator%(const uuid &other) const { return uuid{val % other.val}; }

  friend std::istream &operator>>(std::istream &is, uuid &hash) {
    is >> hash.val;
    return is;
  }

  friend std::ostream &operator<<(std::ostream &os, const uuid &hash) {
    os << hash.string();
    return os;
  }
};
}  // namespace chord

using uuid_t = chord::uuid;
