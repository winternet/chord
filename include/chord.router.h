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

#define BITS 256

#define ROUTER_LOG(level) LOG(level) << "[router] "

namespace chord {

struct Router {

  std::mutex mtx;
  std::map<uuid_t, endpoint_t> routes;

  std::array<uuid_t*, BITS> predecessors;
  std::array<uuid_t*, BITS> successors;

  explicit Router(const chord::Router&) = delete;

  explicit Router(chord::Context &context)
      : context{context} {
    std::fill(std::begin(predecessors), std::end(predecessors), nullptr);
    std::fill(std::begin(successors), std::end(successors), nullptr);
    routes[context.uuid()] = context.bind_addr;
    context.set_router(this);
  }

  virtual ~Router() {
    cleanup();
  }

  void cleanup() {
    for (auto pred:predecessors) delete pred;
    for (auto succ:successors) delete succ;
  }

  void reset() {
    cleanup();

    routes[context.uuid()] = context.bind_addr;
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

  void set_successor(const size_t index, const uuid_t &uuid, const endpoint_t &endpoint) {
    std::lock_guard<std::mutex> lock(mtx);
    ROUTER_LOG(info) << "set_successor[" << index << "][" << uuid << "] = " << endpoint;
    routes[uuid] = endpoint;
    delete successors[index];
    successors[index] = new uuid_t{uuid};
  }

  void set_predecessor(const size_t index, const uuid_t &uuid, const endpoint_t &endpoint) {
    std::lock_guard<std::mutex> lock(mtx);
    ROUTER_LOG(info) << "set_predecessor[" << index << "][" << uuid << "] = " << endpoint;
    routes[uuid] = endpoint;
    delete predecessors[index];
    predecessors[index] = new uuid_t{uuid};
  }

  void reset_predecessor(const size_t index) {
    std::lock_guard<std::mutex> lock(mtx);
    ROUTER_LOG(info) << "reset_predecessor[" << index << "]";
    uuid_t *pred = predecessors[index];

    if(pred == nullptr) return;

    routes.erase(*pred);
    delete pred;
    predecessors[index] = nullptr;
  }

  void reset_successor(const size_t index) {
    std::lock_guard<std::mutex> lock(mtx);
    ROUTER_LOG(info) << "reset_successor[" << index << "]";
    uuid_t *succ = successors[index];

    if(succ == nullptr) return;

    routes.erase(*succ);
    delete succ;
    successors[index] = nullptr;
  }

  void reset(const uuid_t uuid) {
    std::lock_guard<std::mutex> lock(mtx);

    ROUTER_LOG(info) << "reset_successor " << uuid << "@...";
    //routes.erase(uuid);

    // TODO(muffin): refactor
    for (int i = 0; i < BITS; i++) {
      uuid_t* succ = successors[i];
      if (succ!=nullptr && *succ==uuid) {
        ROUTER_LOG(info) << "succ=" << succ << ", uuid=" << &uuid << ", val= " << uuid;
        delete succ;
        successors[i] = nullptr;
      }
    }
    for (int i = 0; i < BITS; i++) {
      uuid_t* pred = predecessors[i];
      if (pred!=nullptr && *pred==uuid) {
        delete pred;
        predecessors[i] = nullptr;
      }
    }
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

  uuid_t *predecessor() {
    //cppcheck-suppress CastIntegerToAddressAtReturn
    return predecessors[0];
  }

  uuid_t closest_preceding_node(const uuid_t &uuid) {
    const uuid_t *direct_predecessor = nullptr;
    const uuid_t *max_predecessor = &context.uuid();
    {
      int m = BITS;
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

    ROUTER_LOG(info) << "no closest preceding node found, returning self " << context.uuid();
    return context.uuid();
  }

  friend std::ostream &operator<<(std::ostream &os, Router &router) {
    for (int i = 0; i < 8; i++) {
      os << "\n::router [successor  ][" << i << "] "
         << (router.successors[i]==nullptr ? std::string("<unknown>") : std::string(*router.successors[i]));
    }
    os << "\n::router [predecessor] "
       << (router.predecessor()!=nullptr ? std::string(*router.predecessor()) : std::string("<unknown>"));
    return os;
  }

 private:
  chord::Context &context;
};
} //namespace chord
