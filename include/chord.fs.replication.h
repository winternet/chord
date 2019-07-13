#pragma once

#include <cstdint>
#include <string>
#include <utility>

namespace chord {
namespace fs {

struct Replication {

  static constexpr auto MAX_REPL_CNT = 10;
  static const Replication NONE;

  // if count == 1, index == 0
  std::uint32_t index;
  std::uint32_t count;  
  
  Replication();
  Replication(const Replication& repl);
  Replication(std::uint32_t count);
  Replication(std::uint32_t index, std::uint32_t count);

  Replication& operator=(Replication rhs);
  Replication& operator++();
  Replication operator++(int);
  Replication& operator--();
  Replication operator--(int);

  bool operator==(const Replication& other) const;

  bool operator<(const Replication& other) const;

  bool has_next() const;

  operator bool() const;

  std::string string() const;

};

} // namespace fs
} // namespace chord
