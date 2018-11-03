#pragma once

#include <string>
#include <utility>

namespace chord {
namespace fs {

struct Replication {

  static constexpr auto MAX_REPL_CNT = 10;

  // if case == 1, index == 0
  unsigned int index;
  unsigned int count;  
  
  Replication() : index{0}, count{1} {}
  Replication(const Replication& repl) : index{repl.index}, count{repl.count} {}
  Replication(unsigned int count) : index{0}, count{count} {}
  Replication(unsigned int index, unsigned int count) : index{index}, count{count} {}

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

  std::string string() const {
    return "["+std::to_string(index)+" of "+std::to_string(count)+"]";
  }

};

} // namespace fs
} // namespace chord
