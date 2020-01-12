#include "chord.router.h"

#include <algorithm>
#include <iterator>
#include <ostream>
#include <memory>
#include <vector>
#include <string>
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
  //for(size_t idx=0; idx < BITS; ++idx) {
  //  const auto finger = calc_predecessor_uuid_for_index(idx);
  //  successors.push_back({finger, nullptr});
  //}
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

//size_t Router::size() const {
//  std::lock_guard<mutex_t> lock(mtx);
//  return routes.size();
//}
//
//optional<node> Router::successor(size_t idx) const {
//  auto succ = successors[idx];
//  if (succ) return node{*succ, routes.at(*succ)};
//  return {};
//}
//
//optional<node> Router::predecessor(size_t idx) const {
//  auto pred = predecessors[idx];
//  if (pred) return node{*pred, routes.at(*pred)};
//  return {};
//}
//
//void Router::replace_predecessor(const chord::node& old_node, const chord::node& new_node) {
//  std::lock_guard<mutex_t> lock(mtx);
//  logger->info("replace_predecessor {} with {}", old_node, new_node);
//
//  routes.erase(old_node.uuid);
//  routes[new_node.uuid] = new_node.endpoint;
//
//  std::for_each(predecessors.begin(), predecessors.end(), [&](optional<uuid_t>& n) {
//      if(n && n.value() == old_node.uuid) {
//        n = new_node.uuid;
//      }
//  });
//}

uuid Router::calc_successor_uuid_for_index(const uuid& self, const size_t i) {
  namespace mp = boost::multiprecision;
  const auto _pow = mp::pow(chord::uuid::value_t{2}, static_cast<unsigned int>(i));
  const auto _mod = std::numeric_limits<chord::uuid::value_t>::max();
  return (self + _pow) % _mod;
}

uuid Router::calc_successor_uuid_for_index(const size_t i) const {
  return Router::calc_successor_uuid_for_index(context.uuid(), i);
}

//uuid Router::calc_predecessor_uuid_for_index(const uuid& self, const size_t i) {
//  namespace mp = boost::multiprecision;
//  const auto _pow = mp::pow(chord::uuid::value_t{2}, static_cast<unsigned int>(i));
//  const auto _mod = std::numeric_limits<chord::uuid::value_t>::max();
//  return (self - _pow) % _mod;
//}
//
//uuid Router::calc_predecessor_uuid_for_index(const size_t i) const {
//  return Router::calc_predecessor_uuid_for_index(context.uuid(), i);
//}

//bool Router::update_successor(const chord::node& old_node, const chord::node& new_node) {
//  std::lock_guard<mutex_t> lock(mtx);
//  logger->info("update_successor {} with {}", old_node, new_node);
//
//  routes[new_node.uuid] = new_node.endpoint;
//  bool updated = false;
//  for(size_t i = 0; i < BITS; ++i) {
//    const auto finger = calc_node_for_index(i);
//    if(finger == old_node.uuid || finger.between(old_node.uuid, new_node.uuid)) {
//      logger->info("update_successor: updating successor at {} with {}", i, new_node);
//      successors[i] = {new_node.uuid};
//      updated = true;
//    }
//  }
//  logger->info("[dump] {}", *this);
//  return updated;
//}
//
//void Router::replace_successor(const chord::node& old_node, const chord::node& new_node) {
//  std::lock_guard<mutex_t> lock(mtx);
//  logger->info("replace_successor {} with {}", old_node, new_node);
//
//  routes.erase(old_node.uuid);
//  routes[new_node.uuid] = new_node.endpoint;
//
//  std::for_each(successors.begin(), successors.end(), [&](optional<uuid_t>& n) {
//      if(n && n.value() == old_node.uuid) {
//        n = new_node.uuid;
//      }
//  });
//}
//
//void Router::set_successor(const size_t index, const chord::node& node) {
//  std::lock_guard<mutex_t> lock(mtx);
//  logger->info("set_successor[{}][{}] = {}", index, node.uuid, node.endpoint);
//  const auto succ = successors[index];
//
//  routes[node.uuid] = node.endpoint;
//  // fill unset preceding nodes
//  for (ssize_t i = index; i >= 0; --i) {
//    if (!successors[i] || *successors[i] == *succ)
//      successors[i] = {node.uuid};
//    else
//      break;
//  }
//}
//
//void Router::set_predecessor(const size_t index, const chord::node& node) {
//  std::lock_guard<mutex_t> lock(mtx);
//  logger->info("set_predecessor[{}][{}] = {}", index, node.uuid, node.endpoint);
//  routes[node.uuid] = node.endpoint;
//  predecessors[index] = {node.uuid};
//}
//
//void Router::reset(const uuid_t& uuid) {
//  std::lock_guard<mutex_t> lock(mtx);
//
//  logger->info("reset {}@...", uuid);
//
//  // replacement to fill holes in successors
//  optional<uuid_t> replacement;
//  for (ssize_t i = successors.size() - 1; i >= 0 && !replacement; --i) {
//    auto succ = successors[i];
//    if(!succ) continue;
//
//    if (*succ != uuid) {
//      replacement = succ;
//    }
//  }
//
//  if (!replacement) {
//    logger->debug("could not find a replacement for {}", uuid);
//  } else {
//    logger->debug("replacement for {} is {}", uuid, *replacement);
//  }
//  for (size_t i = 0; i < successors.size(); ++i) {
//    auto succ = successors[i];
//    if (!succ) continue;
//    if (*succ == uuid) {
//      if (!replacement) {
//        successors[i] = {};
//      } else {
//        successors[i] = replacement;
//      }
//    }
//  }
//
//  // TODO replacement to fill holes in predecessors
//  replacement = {};
//  if (predecessors[0] && *predecessors[0] == uuid) {
//    predecessors[0] = {};
//  }
//}
//
//void Router::fill_successors(const chord::node& node) {
//  std::lock_guard<mutex_t> lock(mtx);
//  routes[node.uuid] = node.endpoint;
//  std::fill(std::begin(successors), std::end(successors), node.uuid);
//}
//
//void Router::fill_predecessors(const chord::node& node) {
//  std::lock_guard<mutex_t> lock(mtx);
//  routes[node.uuid] = node.endpoint;
//  std::fill(std::begin(predecessors), std::end(predecessors), node.uuid);
//}
//
//
//void Router::reset(const node& n) {
//  reset(n.uuid);
//  // for the rare case the successor failed
//  // and only the predecessor known...
//  if(!has_successor() && predecessor()) {
//    const auto pred = predecessor()->uuid;
//    std::fill(std::begin(successors), std::end(successors), pred);
//  }
//// for the rare case the predecessor failed
//// and only the successor is known...
////if(!has_predecessor() && has_successor()) {
////  const auto succ = predecessor(BITS-1);
////}
//}

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
    //if(!it->valid())
    //  changed |= successors.modify(it, change_node(insert));

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
  RouterEntry replacement = {calc_successor_uuid_for_index(BITS), context.node()};

  for(auto it=boost::rbegin(successors); it != boost::rend(successors); ++it) {
    if(it->node() == node || !it->valid()) {
      changed |= successors.modify(std::prev(it.base()), change_node(*replacement._node));
    }
    replacement = *it;
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

std::ostream& operator<<(std::ostream &os, const Router &router) {
  return router.print(os);
  //size_t beg = 0;
  //for (size_t i=1; i < Router::BITS; i++) {
  //  const auto succ_a = router.successor(i-1);
  //  const auto succ_b = router.successor(i);
  //  auto beg_a = (!succ_a ? std::string{"<unknown>"} : succ_a->string());
  //  auto beg_b = (!succ_b ? std::string{"<unknown>"} : succ_b->string());
  //  if(beg_a != beg_b) {
  //    os << "\n::router[successor][" << beg << ".." << i-1 << "] " << beg_a;
  //    beg=i;
  //  }
  //}
  //if (beg != Router::BITS) {
  //  const auto succ_b = router.successor(beg);
  //  auto beg_b = (!succ_b ? std::string{"<unknown>"} : succ_b->string());
  //  os << "\n::router[successor][" << beg << ".." << Router::BITS-1 << "] " << beg_b;
  //}
  //os << "\n::router [predecessor] ";
  //if(router.predecessor()) os << *router.predecessor();
  //else os << "<unknown>";
  //return os;
}
} // namespace chord
