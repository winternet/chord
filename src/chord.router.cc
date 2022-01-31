#include "chord.router.h"

#include <algorithm>
#include <set>
#include <array>
#include <iterator>
#include <ostream>
#include <memory>
#include <string>
#include <optional>

#include "chord.context.h"
#include "chord.log.factory.h"
#include "chord.log.h"
#include "chord.node.h"
#include "chord.types.h"
#include "chord.uuid.h"

namespace chord {

Router::Router(chord::Context &context) noexcept
  : context{context}, logger{context.logging.factory().get_or_create(logger_name)} {
    init();
    context.set_router(this);
}
Router::~Router() noexcept {
  init();
  spdlog::drop(logger_name);
}

void Router::init() {
  std::scoped_lock<mutex_t> lock(mtx);
  for(size_t idx=0; idx < BITS; ++idx) {
    const auto finger = calc_successor_uuid_for_index(idx);
    successors[idx] = {finger, {}};
  }
}

void Router::cleanup() {
  std::scoped_lock<mutex_t> lock(mtx);
  _predecessor.reset();
  init();
}

void Router::reset() {
  cleanup();
}

bool Router::has_successor() const {
  std::scoped_lock<mutex_t> lock(mtx);
  return successors.front().valid();
}

bool Router::has_predecessor() const {
  std::scoped_lock<mutex_t> lock(mtx);
  return _predecessor.has_value();
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

node Router::successor_or_self() const {
  return successor().value_or(context.node());
}

std::optional<node> Router::successor() const {
  std::scoped_lock<mutex_t> lock(mtx);
  auto succ = successors.front();
  if(succ.valid()) return succ.node();
  return {};
  //return context.node();
}

void Router::update(const std::set<chord::node>& nodes) {
  std::scoped_lock<mutex_t> lock(mtx);
  std::for_each(nodes.begin(), nodes.end(), [this](const chord::node& n){this->update(n);});
}

bool Router::update(const chord::node& insert) {
  //std::scoped_lock<mutex_t> lock(mtx);
  bool changed = false;

  if(insert == context.node()) return false;

  std::optional<node> old_successor;
  std::optional<node> old_predecessor;

  for(auto it=successors.begin(); it != successors.end(); ++it) {
    if(uuid::between(it->uuid, insert.uuid, (it->valid() ? it->node().uuid : context.uuid()))) {
      if(successors.begin()->valid() && it == successors.begin()) {
        old_successor = it->node();
      }
      it->_node = insert;
      changed |= true;
    }
  }
  if(!_predecessor 
      || (insert != *_predecessor && uuid::between(_predecessor->uuid, insert.uuid, context.uuid()))) {
    old_predecessor = _predecessor.value_or(context.node());
    _predecessor = insert;
  }

  if(old_successor) event_successor_update(*old_successor, insert);
  if(old_predecessor) event_predecessor_update(*old_predecessor, insert);
  return changed;
}

bool Router::remove(const chord::uuid& uuid, const bool signal) {
  std::scoped_lock<mutex_t> lock(mtx);
  bool changed = false;
  auto replacement = successors.rbegin();

  if(replacement->node().uuid == uuid && replacement->node().uuid != context.uuid())
    //replacement->_node.reset();
    replacement = successors.rend();


  std::optional<node> successor_failed;
  std::optional<node> predecessor_failed;
  {
    auto successor = successors.begin();
    if(successor->valid() && successor->node().uuid == uuid) {
      successor_failed = successor->node();
    }
    if(_predecessor && _predecessor->uuid == uuid) {
      predecessor_failed = _predecessor;
    }
  }

  for(auto it=successors.rbegin(); it != successors.rend(); ++it) {
    if(!it->valid()) continue;

    if(_predecessor && uuid == _predecessor->uuid && it->node().uuid != uuid) {
      _predecessor = it->node();
    }

    if(it->node().uuid == uuid) {
      if(replacement != successors.rend() && replacement->valid()) {
        it->_node = replacement->node();
      } else {
        it->_node.reset();
      }
    }

    if(it->node().uuid != uuid && it->node().uuid != context.uuid())
      replacement = it;
  }

  // not found any replacement - reset
  if(_predecessor && _predecessor->uuid == uuid) {
    _predecessor.reset();
  }

  // emit events after fixing / replacing failed node
  if(signal) {
    if(successor_failed) event_successor_fail(*successor_failed);
    if(predecessor_failed) event_predecessor_fail(*predecessor_failed);
  }

  return changed;
}

bool Router::remove(const chord::node& node, const bool signal) {
  return remove(node.uuid, signal);
}

std::optional<node> Router::predecessor() const {
  std::scoped_lock<mutex_t> lock(mtx);
  return _predecessor;
}

std::vector<node> Router::closest_preceding_nodes(const uuid& uuid) {
  std::scoped_lock<mutex_t> lock(mtx);
  std::vector<node> ret;

  for(auto it=std::crbegin(successors); it != crend(successors); ++it) {
    if(it->valid() && uuid::between(context.uuid(), it->node().uuid, uuid)) {
      const auto node = it->node();
      if(ret.empty() || ret.back() != node) ret.push_back(node);
    }
  }

  if(ret.empty() || ret.front() != context.node()) ret.push_back(context.node());

  return ret;
}

std::optional<node> Router::closest_preceding_node(const uuid_t &uuid) {
  return closest_preceding_nodes(uuid).front();
}

uuid Router::get(const size_t index) const {
  std::scoped_lock<mutex_t> lock(mtx);
  auto it = successors.begin();
  std::advance(it, index);
  return it->uuid;
}

std::set<node> Router::get() const {
  std::scoped_lock<mutex_t> lock(mtx);
  std::set<node> ret;
  for(const auto entry:successors) {
    if(entry.valid()) ret.insert(entry.node());
  }

  return ret;
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

std::ostream& operator<<(std::ostream &os, const Router &router) {
  return router.print(os);
}
} // namespace chord
