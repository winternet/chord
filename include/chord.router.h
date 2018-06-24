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
    std::fill(std::begin(predecessors), std::end(predecessors), nullptr);
    std::fill(std::begin(successors), std::end(successors), nullptr);
    routes[context.uuid()] = context.bind_addr;
    context.set_router(this);
  }

  virtual ~Router() {
    cleanup();
    spdlog::drop(logger_name);
  }

  void cleanup() {
    for (auto pred:predecessors) delete pred;
    for (auto succ:successors) delete succ;
  }

  void reset() {
    cleanup();

    routes[context.uuid()] = context.bind_addr;
  }

  size_t size() const {
    std::lock_guard<std::mutex> lock(mtx);
    return routes.size();
  }

  optional<node> get_successor(const size_t &index) const {
    std::lock_guard<std::mutex> lock(mtx);
    const auto id = *successors[index];
    return node{id, routes.at(id)};
  }

  //TODO fix this use predecessors...
  optional<node> get_predecessor(const size_t &index) const {
    std::lock_guard<std::mutex> lock(mtx);
    const auto id = *successors[index];
    return node{id, routes.at(id)}; //routes[*successors[index]];
  }

  //--- get endpoint to corresponding uuid
  optional<node> get(const uuid_t *uuid) {
    return node{*uuid, routes[*uuid]};
  }

  optional<node> get(const uuid_t &uuid) {
    return node{uuid, routes[uuid]};
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

  void set_successor(const size_t index, const uuid_t &uuid, const endpoint_t &endpoint) {
    std::lock_guard<std::mutex> lock(mtx);
    logger->info("set_successor[{}][{}] = {}", index, uuid, endpoint);
    routes[uuid] = endpoint;
    delete successors[index];
    successors[index] = new uuid_t{uuid};
  }

  void set_predecessor(const size_t index, const uuid_t &uuid, const endpoint_t &endpoint) {
    std::lock_guard<std::mutex> lock(mtx);
    logger->info("set_predecessor[{}][{}] = {}", index, uuid, endpoint);
    routes[uuid] = endpoint;
    delete predecessors[index];
    predecessors[index] = new uuid_t{uuid};
  }

  void reset(const node& n) {
    std::lock_guard<std::mutex> lock(mtx);

    const auto uuid = n.uuid;
    logger->info("reset {}@...", uuid);

    //replacement to fill holes in successors
    uuid_t* replacement = nullptr;
    for(int i=successors.size()-1; i>=0; --i) {
      auto* succ = successors[i];
      if(succ != nullptr) {
        if(*succ == uuid) break;
        replacement = succ;
      }
    }

    if(replacement == nullptr) {
      logger->debug("could not find a replacement for {}", uuid);
    } else {
      logger->debug("replacement for {} is {}", uuid, *replacement);
    }
    for(size_t i=0; i < successors.size(); i++) {
      auto* succ = successors[i];
      if(succ == nullptr) continue;
      if(*succ == uuid) {
        if(replacement == nullptr) {
          successors[i] = nullptr;
        } else {
          successors[i] = new uuid_t{*replacement};
        }
        //successors[i] = replacement != nullptr ? new uuid_t{*replacement} : nullptr;
      }
    }

    //TODO replacement to fill holes in predecessors
    replacement = nullptr;
    if(predecessors[0] != nullptr && *predecessors[0] == uuid) {
      predecessors[0] = nullptr;
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

  optional<node> successor() {
    for (auto succ : successors) {
      //cppcheck-suppress CastIntegerToAddressAtReturn
      if (succ!=nullptr) return node{*succ, routes[*succ]};
    }
    for (auto pred : predecessors) {
      //cppcheck-suppress CastIntegerToAddressAtReturn
      if (pred!=nullptr) return node{*pred, routes[*pred]};
    }
    return node{context.uuid(), routes[context.uuid()]};
  }

  optional<node> predecessor() const {
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
      const auto* succ = *it;
      if(succ != nullptr && succ->between(context.uuid(), uuid)) {
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
      auto beg_a = (router.successors[i-1]==nullptr ? std::string{"<unknown>"} : std::string{*router.successors[i-1]});
      auto beg_o = (router.successors[i]==nullptr ? std::string{"<unknown>"} : std::string{*router.successors[i]});
      if(beg_a != beg_o) {
        os << "\n::router[successor][" << beg << ".." << i-1 << "] " << beg_a;
        beg=i;
      }
    }
    if (beg != BITS) {
      auto beg_o = (router.successors[beg]==nullptr ? std::string{"<unknown>"} : std::string{*router.successors[beg]});
      os << "\n::router[successor][" << beg << ".." << BITS-1 << "] " << beg_o;
    }
    os << "\n::router [predecessor] ";
    if(router.predecessor()) os << *router.predecessor();
    else os << "<unknown>";
    return os;
  }

 private:
  chord::Context &context;
  std::shared_ptr<spdlog::logger> logger;

  mutable std::mutex mtx;
  std::map<uuid_t, endpoint_t> routes;

  std::array<uuid_t*, BITS> predecessors;
  std::array<uuid_t*, BITS> successors;
};
} //namespace chord
