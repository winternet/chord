#include "chord.router.h"

#include <algorithm>
#include <set>
#include <iterator>
#include <ostream>
#include <memory>
#include <vector>
#include <string>
#include <sstream>
#include <boost/multiprecision/cpp_int.hpp>
#include <boost/iterator/reverse_iterator.hpp>

#include "chord.context.h"
#include "chord.log.factory.h"
#include "chord.log.h"
#include "chord.node.h"
#include "chord.types.h"
#include "chord.optional.h"
#include "chord.uuid.h"

namespace chord {

Router::Router(chord::Context &context)
  : context{context}, logger{context.logging.factory().get_or_create(logger_name)} {
    init();
    context.set_router(this);
}
Router::~Router() {
  init();
  spdlog::drop(logger_name);
}

void Router::init() {
  std::lock_guard<mutex_t> lock(mtx);
  for(size_t idx=0; idx < BITS; ++idx) {
    const auto finger = calc_successor_uuid_for_index(idx);
    successors.push_back({finger, {}});
  }
}

void Router::cleanup() {
  std::lock_guard<mutex_t> lock(mtx);
  successors.clear();
  _predecessor._node.reset();
  init();
}

void Router::reset() {
  cleanup();
}

bool Router::has_successor() const {
  std::lock_guard<mutex_t> lock(mtx);
  return successors.front().valid();
}

bool Router::has_predecessor() const {
  std::lock_guard<mutex_t> lock(mtx);
  return _predecessor.valid();
}

uuid Router::calc_successor_uuid_for_index(const uuid& self, const size_t i) {
  namespace mp = boost::multiprecision;
  const auto _pow = mp::pow(chord::uuid::value_t{2}, static_cast<unsigned int>(i));
  const auto _mod = std::numeric_limits<chord::uuid::value_t>::max();
  return (self + _pow) % _mod;
}

uuid Router::calc_successor_uuid_for_index(const size_t i) const {
  return Router::calc_successor_uuid_for_index(context.uuid(), i);
}

optional<node> Router::successor() const {
  auto succ = successors.front();
  if(succ.valid()) return succ.node();
  return {};
}

void Router::update(const std::set<chord::node>& nodes) {
  std::for_each(nodes.begin(), nodes.end(), [this](const chord::node& n){this->update(n);});
}

bool Router::update(const chord::node& insert) {
  bool changed = false;
  for(auto it=successors.begin(); it != successors.end(); ++it) {
    if(uuid::between(it->uuid, insert.uuid, (it->_node ? it->node().uuid : context.uuid()))) {
      changed |= successors.modify(it, change_node(insert));
    }
  }
  if(!_predecessor.valid() || uuid::between(_predecessor.uuid, insert.uuid, context.uuid())) {
    _predecessor._node = insert;
  }
  return changed;
}

bool Router::remove(const chord::node& node) {
  bool changed = false;
  auto replacement = successors.rbegin();

  for(auto it=boost::rbegin(successors); it != boost::rend(successors); ++it) {
    if(!it->valid()) continue;

    if(_predecessor.valid() && node.uuid == _predecessor.node().uuid) {
      _predecessor = *it;
    }

    if(it->node().uuid == node.uuid) {
      if(replacement->valid())
        changed |= successors.modify(std::prev(it.base()), change_node(replacement->node()));
      else 
        changed |= successors.modify(std::prev(it.base()), clear_node());
    }
    replacement = it;
  }

  return changed;
}

optional<node> Router::predecessor() const {
  if(_predecessor.valid()) return _predecessor.node();
  return {};
}

optional<node> Router::closest_preceding_node(const uuid_t &uuid) {
  for(auto it=std::crbegin(successors); it != crend(successors); ++it) {
    if(it->valid() && uuid::between(context.uuid(), it->node().uuid, uuid)) {
      logger->info("closest preceding node for {} found is {}", uuid, it->node());
      return it->node();
    }
  }

  logger->info("no closest preceding node for {} found, returning self {}", uuid, context.uuid());
  return context.node();
}

uuid Router::get(const size_t index) const {
  auto it = successors.begin();
  std::advance(it, index);
  return it->uuid;
}

std::string Router::print() const {
  std::stringstream ss;
  print(ss);
  return ss.str();
}

std::ostream& Router::print(std::ostream& os) const {
  size_t beg = 0, end = 0;
  auto curr = successors.front();
  for(const auto successor:successors) {
    //os << "\nrouter[" << beg++ << "]: "; successor.print(os);
    if(curr.valid() ^ successor.valid() || (curr.valid() && successor.valid() && curr.node() != successor.node())) {
      os << "\nrouter[" << beg << ".." << end-1 << "]: "; curr.print(os);
      curr = successor;
      beg = end;
    }
    ++end;
  }
  os << "\nrouter[" << beg << ".." << end  << "]: "; curr.print(os);
  return os;
}

std::set<node> Router::finger() const {
  std::set<node> finger;
  for(const auto& entry:successors) {
    if(entry.valid())
      finger.insert(entry.node());
  }
  return finger;
}

std::ostream& operator<<(std::ostream &os, const Router &router) {
  return router.print(os);
}
} // namespace chord
