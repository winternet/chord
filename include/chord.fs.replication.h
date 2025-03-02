#pragma once

#include <cstdint>
#include <string>
#include <fmt/ostream.h>
#include <utility>

namespace chord {
namespace fs {

struct Replication {

  static constexpr auto MAX_REPL_CNT = 10;
  static const Replication NONE;
  static const Replication ALL;

  // if count == 1, index == 0
  std::uint32_t index;
  std::uint32_t count;  
  
  Replication() noexcept;
  Replication(const Replication& repl) noexcept;
  explicit Replication(std::uint32_t count) noexcept;
  Replication(std::uint32_t index, std::uint32_t count) noexcept;

  Replication& operator=(Replication rhs);
  Replication& operator++();
  const Replication operator++(int);
  Replication& operator--();
  const Replication operator--(int);

  bool operator==(const Replication&) const;
  bool operator!=(const Replication&) const;

  bool operator<(const Replication&) const;

  bool has_next() const;

  operator bool() const;

  std::string string() const;

};

} // namespace fs
} // namespace chord

template<> struct fmt::formatter<chord::fs::Replication> : ostream_formatter {};
