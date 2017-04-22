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

  Router(Context* context)
    : context { context }
  {
    reset();
  }

  std::map<uuid_t, endpoint_t> routes;

  boost::array<std::shared_ptr<uuid_t>, BITS> predecessors;
  boost::array<std::shared_ptr<uuid_t>, BITS> successors;

  void reset() {
    predecessors = boost::array<std::shared_ptr<uuid_t>, BITS>();
    successors = boost::array<std::shared_ptr<uuid_t>, BITS>();
    set_successor(0, context->uuid(), context->bind_addr);
  }

  endpoint_t get_successor(const size_t& index) {
    return routes[*successors[index]];
  }

  endpoint_t get_predecessor(const size_t& index) {
    return routes[*successors[index]];
  }

  //--- get endpoint to corresponding uuid
  endpoint_t get(const uuid_t& uuid) {
    return routes[uuid];
  }
  endpoint_t get(const std::shared_ptr<uuid_t>& uuid) {
    return routes[*uuid];
  }

  void set_successor(const size_t index, const uuid_t uuid, const endpoint_t endpoint) {
    ROUTER_LOG(info) << "set_successor[" << index << "][" << to_string(uuid) << "] = " << endpoint;
    routes[uuid] = endpoint;
    successors[index] = std::unique_ptr<uuid_t>(new uuid_t(uuid));
  }

  void set_predecessor(const size_t index, const uuid_t uuid, const endpoint_t endpoint) {
    ROUTER_LOG(info) << "set_predecessor[" << index << "][" << to_string(uuid) << "] = " << endpoint;
    routes[uuid] = endpoint;
    predecessors[index] = std::unique_ptr<uuid_t>(new uuid_t(uuid));
  }

  void reset_predecessor(const size_t index) {
    ROUTER_LOG(info) << "reset_predecessor[" << index << "]";
    std::shared_ptr<uuid_t> pred = predecessors[index];
    routes.erase(*pred);
    predecessors[index] = nullptr;
  }

  void reset_successor(const size_t index) {
    ROUTER_LOG(info) << "reset_successor[" << index << "]";
    std::shared_ptr<uuid_t> pred = successors[index];
    routes.erase(*pred);
    successors[index] = nullptr;
  }

  void reset(uuid_t uuid) {
    reset(std::make_shared<uuid_t>(uuid));
  }

  void reset(const std::shared_ptr<uuid_t>& uuid) {
    if( uuid == nullptr ) {
      ROUTER_LOG(warning) << "failed to reset nullptr";
      return;
    }

    ROUTER_LOG(info) << "reset_successor " << *uuid << "@...";
    routes.erase(*uuid);

    // TODO refactor
    for(int i=0; i < BITS; i++) {
      auto succ = successors[i];
      if( succ != nullptr && *succ == *uuid) successors[i] = nullptr;
    }
    for(int i=0; i < BITS; i++) {
      auto pred = predecessors[i];
      if( pred != nullptr && *pred == *uuid) predecessors[i] = nullptr;
    }
  }

  std::shared_ptr<uuid_t> successor() {
    for( auto succ : successors ) {
      if( succ != nullptr ) return succ;
    }
    for( auto pred : predecessors ) {
      if( pred != nullptr ) return pred;
    }
    return std::make_shared<uuid_t>(context->uuid());
    //return successors[0];
  }

  std::shared_ptr<uuid_t> predecessor() {
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
    ROUTER_LOG(info) << "no closest preceding node found, returning self " << context->uuid();
    return context->uuid();
  }

  friend std::ostream& operator<<(std::ostream& os, Router& router) {
    for( int i=0; i < 8; i++ ) {
      os << "\n::router [successor  ][" << i << "] "
         << (router.successors[i] != nullptr ? to_string(*router.successors[i]) + "@" + router.get_successor(i) : "<unknown>");
    }
    //os  << "\n::router [successor  ] " << (router.successor() != nullptr ? to_string(*router.successor()) : "<unknown>") 
    //    << "\n::router [predecessor] " << (router.predecessor() != nullptr ? to_string(*router.predecessor()) : "<unknown>");
    return os;
  }

private:
  Context* context;
};
