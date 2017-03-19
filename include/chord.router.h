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

  std::shared_ptr<uuid_t> successor() {
    return successors[0];
  }

  std::shared_ptr<uuid_t> predecessor() {
    return predecessors[0];
  }

  uuid_t closest_preceding_node(const uuid_t& uuid) {
    int m = BITS;
    do {
      m--;
    //for( size_t m = BITS-1; m >= 0; m-- ) {
      std::cerr << "\nm = " << m;
      auto candidate = successors[m];
      if( m == 0 ) std::cerr << "\n\nCLOSEST_PREC_NODE m == 0\n\n";
      if( candidate == nullptr ) continue;

      std::cerr << "\nCLOSEST_PREC_NODE: cand " << *candidate << " uuid " << uuid << "cand < uuid?";
      if( *candidate < uuid )
        return *candidate;
    } while( m > 0 );
    return context->uuid();
  }

  friend std::ostream& operator<<(std::ostream& os, Router& router) {
    os  << "\n::router [successor  ] " << (router.successor() != nullptr ? to_string(*router.successor()) : "<unknown>") 
        << "\n::router [predecessor] " << (router.predecessor() != nullptr ? to_string(*router.predecessor()) : "<unknown>");
    return os;
  }

private:
  Context* context;
};
