#include "chord.fs.replication.h"

#include <cstdint>
#include <string>
#include <utility>
#include <limits>

namespace chord {
namespace fs {
  
Replication::Replication() : index{0}, count{1} {}
Replication::Replication(const Replication& repl) : index{repl.index}, count{repl.count} {}
Replication::Replication(std::uint32_t count) : index{0}, count{count} {}
Replication::Replication(std::uint32_t index, std::uint32_t count) : index{index}, count{count} {}

Replication& Replication::operator=(Replication rhs) {
    std::swap(rhs.count, count);
    std::swap(rhs.index, index);
    return *this;
  }

  Replication& Replication::operator++() {
    ++index;
    return *this;
  }

  const Replication Replication::operator++(int) {
    Replication tmp{*this};
    ++(*this);
    return tmp;
  }

  Replication& Replication::operator--() {
    if(index > 0) --index;
    return *this;
  }

  const Replication Replication::operator--(int) {
    Replication tmp{*this};
    --(*this);
    return tmp;
  }

  bool Replication::operator==(const Replication& other) const {
    return other.index == index && other.count == count;
  }

  bool Replication::operator!=(const Replication& other) const {
    return !(*this == other);
  }

  bool Replication::operator<(const Replication& other) const {
    return count < other.count;
  }

  bool Replication::has_next() const {
    return index+1 < count;
  }

  Replication::operator bool() const {
    return index < count && count > 1;
  }

  std::string Replication::string() const {
    return "["+std::to_string(index)+" of "+std::to_string(count)+"]";
  }


const Replication Replication::NONE = Replication();
const Replication Replication::ALL = Replication(std::numeric_limits<std::uint32_t>::max());

} // namespace fs
} // namespace chord
