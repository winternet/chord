#pragma once

#include <boost/array.hpp>
#include <map>
#include <memory>
#include <mutex>
#include <string>

#include "chord.context.h"
#include "chord.log.h"
#include "chord.types.h"
#include "chord.uuid.h"

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

  endpoint_t get_successor(const size_t &index) {
    std::lock_guard<std::mutex> lock(mtx);
    return routes[*successors[index]];
  }

  endpoint_t get_predecessor(const size_t &index) {
    std::lock_guard<std::mutex> lock(mtx);
    return routes[*successors[index]];
  }

  //--- get endpoint to corresponding uuid
  endpoint_t get(const uuid_t *uuid) {
    return routes[*uuid];
  }

  endpoint_t get(const uuid_t &uuid) {
    return routes[uuid];
  }

  const uuid_t* successor(size_t idx) const {
    return successors[idx];
  }

  const uuid_t* predecessor(size_t idx) const {
    return predecessors[idx];
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

  //void reset_predecessor(const size_t index) {
  //  std::lock_guard<std::mutex> lock(mtx);
  //  logger->info("reset_predecessor[{}]", index);
  //  uuid_t *pred = predecessors[index];

  //  if(pred == nullptr) return;

  //  routes.erase(*pred);
  //  delete pred;
  //  predecessors[index] = nullptr;
  //}

  //void reset_successor(const size_t index) {
  //  std::lock_guard<std::mutex> lock(mtx);
  //  logger->info("reset_successor[{}]", index);
  //  uuid_t *succ = successors[index];

  //  if(succ == nullptr) return;

  //  routes.erase(*succ);
  //  delete succ;
  //  successors[index] = nullptr;
  //}

  void reset(const uuid_t& uuid) {
    std::lock_guard<std::mutex> lock(mtx);

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
      if(*succ == uuid) successors[i] = replacement != nullptr ? new uuid_t{*replacement} : nullptr;
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

  const uuid_t *successor() {
    for (auto succ : successors) {
      //cppcheck-suppress CastIntegerToAddressAtReturn
      if (succ!=nullptr) return succ;
    }
    for (auto pred : predecessors) {
      //cppcheck-suppress CastIntegerToAddressAtReturn
      if (pred!=nullptr) return pred;
    }
    return &(context.uuid());
  }

  const uuid_t *predecessor() const {
    //cppcheck-suppress CastIntegerToAddressAtReturn
    return predecessors[0];
  }

  uuid_t *predecessor() {
    //cppcheck-suppress CastIntegerToAddressAtReturn
    return predecessors[0];
  }

  uuid_t closest_preceding_node(const uuid_t &uuid) {
    const uuid_t *direct_predecessor = nullptr;
    const uuid_t *max_predecessor = &context.uuid();
    {
      auto m = BITS;
      do {
        m--;
        auto candidate = successors[m];

        if (candidate == nullptr) continue;

        // max predecessor
        if (*candidate > *max_predecessor) {
          max_predecessor = candidate;
        }

        // try to find predecessor
        if (*candidate < uuid) {
          direct_predecessor = candidate;
          break;
        }
      } while (m > 0);
    }

    if (direct_predecessor != nullptr) return *direct_predecessor;
    if (max_predecessor != nullptr) return *max_predecessor;

    logger->info("no closest preceding node found, returning self {}", context.uuid());
    return context.uuid();
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
    os << "\n::router [predecessor] "
       << (router.predecessor()!=nullptr ? std::string(*router.predecessor()) : std::string("<unknown>"));
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
