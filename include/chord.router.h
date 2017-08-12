#pragma once

#include <map>
#include <memory>
#include <string>
#include <boost/array.hpp>

#include "chord.types.h"
#include "chord.context.h"
#include "chord.uuid.h"
#include "chord.log.h"

#define BITS 256

#define ROUTER_LOG(level) LOG(level) << "[router] "


struct Router {

  std::map<uuid_t, endpoint_t> routes;

  std::array<uuid_t*, BITS> predecessors;
  std::array<uuid_t*, BITS> successors;


  Router(Context* context)
    : context { context }
  {
    std::fill(std::begin(predecessors), std::end(predecessors), nullptr);
    std::fill(std::begin(successors), std::end(successors), nullptr);
    //for(auto i=0; i < BITS; i++) successors[i] = nullptr;
    //for(auto i=0; i < BITS; i++) predecessors[i] = nullptr;
    routes[context->uuid()] = context->bind_addr;
    context->set_router(this);
  }

  virtual ~Router() {
    cleanup();
  }

  void cleanup() {
    for(auto pred:predecessors)
      if(pred != nullptr) delete pred;
    for(auto succ:successors)
      if(succ != nullptr) delete succ;
  }

  void reset() {
    cleanup();

    routes[context->uuid()] = context->bind_addr;
  }

  endpoint_t get_successor(const size_t& index) {
    return routes[*successors[index]];
  }

  endpoint_t get_predecessor(const size_t& index) {
    return routes[*successors[index]];
  }

  //--- get endpoint to corresponding uuid
  endpoint_t get(const uuid_t* uuid) {
    return routes[*uuid];
  }
  endpoint_t get(const uuid_t& uuid) {
    return routes[uuid];
  }

  void set_successor(const size_t index, const uuid_t uuid, const endpoint_t endpoint) {
    ROUTER_LOG(info) << "set_successor[" << index << "][" << uuid << "] = " << endpoint;
    routes[uuid] = endpoint;
    delete successors[index];
    successors[index] = new uuid_t(uuid);
  }

  void set_predecessor(const size_t index, const uuid_t uuid, const endpoint_t endpoint) {
    ROUTER_LOG(info) << "set_predecessor[" << index << "][" << uuid << "] = " << endpoint;
    routes[uuid] = endpoint;
    delete predecessors[index];
    predecessors[index] = new uuid_t(uuid);
  }

  void reset_predecessor(const size_t index) {
    ROUTER_LOG(info) << "reset_predecessor[" << index << "]";
    uuid_t* pred = predecessors[index];
    routes.erase(*pred);
    delete pred;
    predecessors[index] = nullptr;
  }

  void reset_successor(const size_t index) {
    ROUTER_LOG(info) << "reset_successor[" << index << "]";
    uuid_t* succ = successors[index];
    routes.erase(*succ);
    delete succ;
    successors[index] = nullptr;
  }

  //void reset(uuid_t uuid) {
  //  reset(new uuid_t(uuid)));
  //}

  void reset(const uuid_t* uuid) {
    if( uuid == nullptr ) {
      ROUTER_LOG(warning) << "failed to reset nullptr";
      return;
    }

    ROUTER_LOG(info) << "reset_successor " << *uuid << "@...";
    //routes.erase(uuid);

    // TODO refactor
    for(int i=0; i < BITS; i++) {
      auto succ = successors[i];
      if( succ != nullptr && *succ == *uuid) {
        delete succ;
        successors[i] = nullptr;
      }
    }
    for(int i=0; i < BITS; i++) {
      auto pred = predecessors[i];
      if( pred != nullptr && *pred == *uuid) {
        delete pred;
        predecessors[i] = nullptr;
      }
    }
  }

  uuid_t* successor() {
    for( auto succ : successors ) {
      if( succ != nullptr ) return succ;
    }
    for( auto pred : predecessors ) {
      if( pred != nullptr ) return pred;
    }
    return &(context->uuid());
  }

  uuid_t* predecessor() {
    return predecessors[0];
  }

  uuid_t closest_preceding_node(const uuid_t& uuid) {
    int m = BITS;
    do {
      m--;
      auto candidate = successors[m];

      if( candidate == nullptr ) continue;

      if( *candidate < uuid )
        return *candidate;
    } while( m > 0 );

    for( auto candidate : successors ) {
      if( candidate == nullptr ) continue;
      if( *candidate > uuid ) return *candidate;
    }

    ROUTER_LOG(info) << "no closest preceding node found, returning self " << context->uuid();
    return context->uuid();
  }

  friend std::ostream& operator<<(std::ostream& os, Router& router) {
    for( int i=0; i < 8; i++ ) {
      os << "\n::router [successor  ][" << i << "] "
         << (router.successors[i] != nullptr ? *router.successors[i] + "@" + router.get_successor(i) : "<unknown>");
    }
    //os  << "\n::router [successor  ] " << (router.successor() != nullptr ? *router.successor() : "<unknown>") 
    os << "\n::router [predecessor] " << (router.predecessor() != nullptr ? *router.predecessor() : "<unknown>");
    return os;
  }

private:
  Context* context;
};
