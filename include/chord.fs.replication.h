#pragma once

#include <cstdint>
#include <string>
#include <utility>

namespace chord {
namespace fs {

struct Replication {

  static constexpr auto MAX_REPL_CNT = 10;

  // if case == 1, index == 0
  std::uint32_t index;
  std::uint32_t count;  
  
  Replication() : index{0}, count{1} {}
  Replication(const Replication& repl) : index{repl.index}, count{repl.count} {}
  Replication(std::uint32_t count) : index{0}, count{count} {}
  Replication(std::uint32_t index, std::uint32_t count) : index{index}, count{count} {}

  Replication& operator=(Replication rhs) {
    std::swap(rhs.count, count);
    std::swap(rhs.index, index);
    return *this;
  }

  Replication& operator++() {
    ++index;
    return *this;
  }

  Replication operator++(int) {
    Replication tmp{*this};
    ++(*this);
    return tmp;
  }

  Replication& operator--() {
    if(index > 0) --index;
    return *this;
  }

  Replication operator--(int) {
    Replication tmp{*this};
    --(*this);
    return tmp;
  }

  bool operator==(const Replication& other) const {
    return other.index == index && other.count == count;
  }

  bool operator<(const Replication& other) const {
    return count < other.count;
  }

  operator bool() const {
    return index < count;
  }

  std::string string() const {
    return "["+std::to_string(index)+" of "+std::to_string(count)+"]";
  }

};

} // namespace fs
} // namespace chord
