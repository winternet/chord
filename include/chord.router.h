#pragma once

#include <boost/array.hpp>
#include <map>
#include <memory>
#include <mutex>
#include <string>

#include "chord.node.h"
#include "chord.context.h"
#include "chord.log.h"
#include "chord.types.h"
#include "chord.uuid.h"
#include "chord.optional.h"


namespace chord {

struct Router {

  static constexpr size_t BITS = 256;
  static constexpr auto logger_name = "chord.router";


  explicit Router(const chord::Router&) = delete;

  explicit Router(chord::Context &context)
      : context{context}, logger{log::get_or_create(logger_name)} {
    cleanup();
    routes[context.uuid()] = context.bind_addr;
    context.set_router(this);
  }

  virtual ~Router() {
    cleanup();
    spdlog::drop(logger_name);
  }

  void cleanup() {
    std::fill(std::begin(predecessors), std::end(predecessors), optional<uuid>{});
    std::fill(std::begin(successors), std::end(successors), optional<uuid>{});
  }

  void reset() {
    std::lock_guard<mutex_t> lock(mtx);
    cleanup();
    routes[context.uuid()] = context.bind_addr;
  }

  bool has_successor() const {
    std::lock_guard<mutex_t> lock(mtx);
    return std::any_of(std::begin(successors), std::end(successors), [](const optional<uuid>& succ) {return succ;});
  }

  /**
   * get the amount of known routes.
   *
   * note that this does _neither_ return the size of the
   * chord ring nor the currently known routes, it just returns
   * all routes it once knew.
   */
  size_t size() const {
    std::lock_guard<mutex_t> lock(mtx);
    return routes.size();
  }

  optional<node> successor(size_t idx) const {
    auto succ = successors[idx];
    if(succ) return node{*succ, routes.at(*succ)};
    return {};
  }

  optional<node> predecessor(size_t idx) const {
    auto pred = predecessors[idx];
    if(pred) return node{*pred, routes.at(*pred)};
    return {};
  }

  void set_successor(const size_t index, const chord::node& node) {
    std::lock_guard<mutex_t> lock(mtx);
    logger->info("set_successor[{}][{}] = {}", index, node.uuid, node.endpoint);
    const auto succ = successors[index];

    routes[node.uuid] = node.endpoint;
    // fill unset preceding nodes
    for (int i = index; i >= 0; --i) {
      if(!successors[i] || *successors[i] == *succ)
        successors[i] = {node.uuid};
      else break;
    }
  }

  void set_predecessor(const size_t index, const chord::node& node) {
    std::lock_guard<mutex_t> lock(mtx);
    logger->info("set_predecessor[{}][{}] = {}", index, node.uuid, node.endpoint);
    routes[node.uuid] = node.endpoint;
    predecessors[index] = {node.uuid};
  }

  void reset(const uuid_t& uuid) {
    std::lock_guard<mutex_t> lock(mtx);

    logger->info("reset {}@...", uuid);

    //replacement to fill holes in successors
    optional<uuid_t> replacement;
    for(int i=successors.size()-1; i>=0; --i) {
      auto succ = successors[i];
      if(succ) {
        if(*succ == uuid) break;
        replacement = succ;
      }
    }

    if(!replacement) {
      logger->debug("could not find a replacement for {}", uuid);
    } else {
      logger->debug("replacement for {} is {}", uuid, *replacement);
    }
    for(size_t i=0; i < successors.size(); i++) {
      auto succ = successors[i];
      if(!succ) continue;
      if(*succ == uuid) {
        if(!replacement) {
          successors[i] = {};
        } else {
          successors[i] = replacement;
        }
      }
    }

    //TODO replacement to fill holes in predecessors
    replacement = {};
    if(predecessors[0] && *predecessors[0] == uuid) {
      predecessors[0] = {};
    }
    //for(int i=predecessors.size()-1; i>=0; --i) {
    //  auto* pred = predecessors[i];
    //  if(pred != nullptr) {
    //    if(*pred == uuid) break;
    //    replacement = pred;
    //  }
    //}

    //for(size_t i=0; i < predecessors.size(); i++) {
    //  auto* pred = predecessors[i];
    //  if(pred == nullptr) continue;
    //  if(*pred == uuid) pred = replacement;
    //}

    // TODO refactor
    //for (size_t i = 0; i < BITS; i++) {
    //  uuid_t* succ = successors[i];
    //  if (succ!=nullptr && *succ==uuid) {
    //    //logger->info("succ={:p}, uuid={:p}, val={}", succ, uuid, uuid);
    //    delete succ;
    //    successors[i] = nullptr;
    //  }
    //}
    //for (size_t i = 0; i < BITS; i++) {
    //  uuid_t* pred = predecessors[i];
    //  if (pred!=nullptr && *pred==uuid) {
    //    delete pred;
    //    predecessors[i] = nullptr;
    //  }
    //}
  }
  void reset(const node& n) {
    reset(n.uuid);
  }

  optional<node> successor() {
    for (auto succ : successors) {
      //cppcheck-suppress CastIntegerToAddressAtReturn
      if (succ) return node{*succ, routes[*succ]};
    }
    for (auto pred : predecessors) {
      //cppcheck-suppress CastIntegerToAddressAtReturn
      if (pred) return node{*pred, routes[*pred]};
    }
    return node{context.uuid(), routes[context.uuid()]};
  }

  const optional<node> predecessor() const {
    //cppcheck-suppress CastIntegerToAddressAtReturn
    auto pre = predecessors[0];
    if(pre) return node{*predecessors[0], routes.at(*predecessors[0])};
    return {};
  }

  optional<node> predecessor() {
    //cppcheck-suppress CastIntegerToAddressAtReturn
    auto pre = predecessors[0];
    if(pre) return node{*predecessors[0], routes.at(*predecessors[0])};
    return {};
  }

  optional<node> closest_preceding_node(const uuid_t &uuid) {
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

  friend std::ostream &operator<<(std::ostream &os, const Router &router) {
    size_t beg = 0;
    for (size_t i=1; i < BITS; i++) {
      auto beg_a = (!(router.successors[i-1]) ? std::string{"<unknown>"} : std::string{*router.successors[i-1]});
      auto beg_o = (!(router.successors[i]) ? std::string{"<unknown>"} : std::string{*router.successors[i]});
      if(beg_a != beg_o) {
        os << "\n::router[successor][" << beg << ".." << i-1 << "] " << beg_a;
        beg=i;
      }
    }
    if (beg != BITS) {
      auto beg_o = (!(router.successors[beg]) ? std::string{"<unknown>"} : std::string{*router.successors[beg]});
      os << "\n::router[successor][" << beg << ".." << BITS-1 << "] " << beg_o;
    }
    os << "\n::router [predecessor] ";
    if(router.predecessor()) os << *router.predecessor();
    else os << "<unknown>";
    return os;
  }

 private:
  using mutex_t = std::recursive_mutex;
  chord::Context &context;
  std::shared_ptr<spdlog::logger> logger;

  mutable mutex_t mtx;
  std::map<uuid_t, endpoint_t> routes;

  std::array<optional<uuid_t>, BITS> predecessors;
  std::array<optional<uuid_t>, BITS> successors;
};
}  // namespace chord
