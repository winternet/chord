#include "chord.router.h"

#include <algorithm>
#include <iterator>
#include <ostream>
#include <memory>
#include <string>
#include <boost/multiprecision/cpp_int.hpp>

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
    cleanup();
    routes[context.uuid()] = context.bind_addr;
    context.set_router(this);
  }
Router::~Router() {
  cleanup();
  spdlog::drop(logger_name);
}

void Router::cleanup() {
  std::fill(std::begin(predecessors), std::end(predecessors), optional<uuid>{});
  std::fill(std::begin(successors), std::end(successors), optional<uuid>{});
}

void Router::reset() {
  std::lock_guard<mutex_t> lock(mtx);
  cleanup();
  routes[context.uuid()] = context.bind_addr;
}

bool Router::has_successor() const {
  std::lock_guard<mutex_t> lock(mtx);
  return std::any_of(std::begin(successors), std::end(successors), [](const optional<uuid>& succ) {return succ;});
}

size_t Router::size() const {
  std::lock_guard<mutex_t> lock(mtx);
  return routes.size();
}

optional<node> Router::successor(size_t idx) const {
  auto succ = successors[idx];
  if (succ) return node{*succ, routes.at(*succ)};
  return {};
}

optional<node> Router::predecessor(size_t idx) const {
  auto pred = predecessors[idx];
  if (pred) return node{*pred, routes.at(*pred)};
  return {};
}

void Router::replace_predecessor(const chord::node& old_node, const chord::node& new_node) {
  std::lock_guard<mutex_t> lock(mtx);
  logger->info("replace_predecessor {} with {}", old_node, new_node);

  routes.erase(old_node.uuid);
  routes[new_node.uuid] = new_node.endpoint;

  std::for_each(predecessors.begin(), predecessors.end(), [&](optional<uuid_t>& n) {
      if(n && n.value() == old_node.uuid) {
        n = new_node.uuid;
      }
  });
}

uuid Router::calc_node_for_index(const uuid& self, const size_t i) {
  const auto _pow = boost::multiprecision::pow(chord::uuid::value_t{2}, static_cast<unsigned int>(i));
  const auto _mod = std::numeric_limits<chord::uuid::value_t>::max();
  return (self + _pow) % _mod;
}

uuid Router::calc_node_for_index(const size_t i) const {
  return Router::calc_node_for_index(context.uuid(), i);
}

bool Router::update_successor(const chord::node& old_node, const chord::node& new_node) {
  std::lock_guard<mutex_t> lock(mtx);
  logger->info("update_successor {} with {}", old_node, new_node);

  routes[new_node.uuid] = new_node.endpoint;
  bool updated = false;
  for(size_t i = 0; i < BITS; ++i) {
    const auto finger = calc_node_for_index(i);
    if(finger == old_node.uuid || finger.between(old_node.uuid, new_node.uuid)) {
      logger->info("update_successor: updating successor at {} with {}", i, new_node);
      successors[i] = {new_node.uuid};
      updated = true;
    }
  }
  return updated;
}

void Router::replace_successor(const chord::node& old_node, const chord::node& new_node) {
  std::lock_guard<mutex_t> lock(mtx);
  logger->info("replace_successor {} with {}", old_node, new_node);

  routes.erase(old_node.uuid);
  routes[new_node.uuid] = new_node.endpoint;

  std::for_each(successors.begin(), successors.end(), [&](optional<uuid_t>& n) {
      if(n && n.value() == old_node.uuid) {
        n = new_node.uuid;
      }
  });
}

void Router::set_successor(const size_t index, const chord::node& node) {
  std::lock_guard<mutex_t> lock(mtx);
  logger->info("set_successor[{}][{}] = {}", index, node.uuid, node.endpoint);
  const auto succ = successors[index];

  routes[node.uuid] = node.endpoint;
  // fill unset preceding nodes
  for (ssize_t i = index; i >= 0; --i) {
    if (!successors[i] || *successors[i] == *succ)
      successors[i] = {node.uuid};
    else
      break;
  }
}

void Router::set_predecessor(const size_t index, const chord::node& node) {
  std::lock_guard<mutex_t> lock(mtx);
  logger->info("set_predecessor[{}][{}] = {}", index, node.uuid, node.endpoint);
  routes[node.uuid] = node.endpoint;
  predecessors[index] = {node.uuid};
}

void Router::reset(const uuid_t& uuid) {
  std::lock_guard<mutex_t> lock(mtx);

  logger->info("reset {}@...", uuid);

  // replacement to fill holes in successors
  optional<uuid_t> replacement;
  for (ssize_t i = successors.size() - 1; i >= 0; --i) {
    auto succ = successors[i];
    if (succ) {
      if (*succ == uuid) break;
      replacement = succ;
    }
  }

  if (!replacement) {
    logger->debug("could not find a replacement for {}", uuid);
  } else {
    logger->debug("replacement for {} is {}", uuid, *replacement);
  }
  for (size_t i = 0; i < successors.size(); ++i) {
    auto succ = successors[i];
    if (!succ) continue;
    if (*succ == uuid) {
      if (!replacement) {
        successors[i] = {};
      } else {
        successors[i] = replacement;
      }
    }
  }

  // TODO replacement to fill holes in predecessors
  replacement = {};
  if (predecessors[0] && *predecessors[0] == uuid) {
    predecessors[0] = {};
  }
}

void Router::reset(const node& n) {
  reset(n.uuid);
}

optional<node> Router::successor() {
  for (auto succ : successors) {
    // cppcheck-suppress CastIntegerToAddressAtReturn
    if (succ) return node{*succ, routes[*succ]};
  }
  for (auto pred : predecessors) {
    // cppcheck-suppress CastIntegerToAddressAtReturn
    if (pred) return node{*pred, routes[*pred]};
  }
  return node{context.uuid(), routes[context.uuid()]};
}

const optional<node> Router::predecessor() const {
  //cppcheck-suppress CastIntegerToAddressAtReturn
  auto pre = predecessors[0];
  if(pre) return node{*predecessors[0], routes.at(*predecessors[0])};
  return {};
}

optional<node> Router::predecessor() {
  //cppcheck-suppress CastIntegerToAddressAtReturn
  auto pre = predecessors[0];
  if(pre) return node{*predecessors[0], routes.at(*predecessors[0])};
  return {};
}

optional<node> Router::closest_preceding_node(const uuid_t &uuid) {
  for(auto it=std::crbegin(successors); it != crend(successors); ++it) {
    const auto succ = *it;
    if(succ && succ->between(context.uuid(), uuid)) {
      logger->info("closest preceding node for {} found is {}", uuid, *succ);
      return node{*succ, routes[*succ]};
    }
  }

  logger->info("no closest preceding node for {} found, returning self {}", uuid, context.uuid());
  return node{context.uuid(), routes[context.uuid()]};
}

std::ostream& operator<<(std::ostream &os, const Router &router) {
  size_t beg = 0;
  for (size_t i=1; i < Router::BITS; i++) {
    const auto succ_a = router.successor(i-1);
    const auto succ_b = router.successor(i);
    auto beg_a = (!succ_a ? std::string{"<unknown>"} : succ_a->string());
    auto beg_b = (!succ_b ? std::string{"<unknown>"} : succ_b->string());
    if(beg_a != beg_b) {
      os << "\n::router[successor][" << beg << ".." << i-1 << "] " << beg_a;
      beg=i;
    }
  }
  if (beg != Router::BITS) {
    const auto succ_b = router.successor(beg);
    auto beg_b = (!succ_b ? std::string{"<unknown>"} : succ_b->string());
    os << "\n::router[successor][" << beg << ".." << Router::BITS-1 << "] " << beg_b;
  }
  os << "\n::router [predecessor] ";
  if(router.predecessor()) os << *router.predecessor();
  else os << "<unknown>";
  return os;
}
} // namespace chord
