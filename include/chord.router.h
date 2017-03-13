#pragma once

#include <map>
#include <string>
#include <boost/array.hpp>

#include "chord.types.h"
#include "chord.context.h"
#include "chord.uuid.h"
#include "chord.log.h"

#define BITS 256

struct Router {

  Router(Context* context)
    : context { context }
  {

  }

  std::map<uuid_t, endpoint_t> routes;

  boost::array<std::unique_ptr<uuid_t>, BITS> predecessors;
  boost::array<std::unique_ptr<uuid_t>, BITS> successors;

  void reset() {
    predecessors = boost::array<std::unique_ptr<uuid_t>, BITS>();
    successors = boost::array<std::unique_ptr<uuid_t>, BITS>();
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

  void set_successor(const size_t index, const uuid_t uuid, const endpoint_t endpoint) {
    routes[uuid] = endpoint;
    successors[index] = std::unique_ptr<uuid_t>(new uuid_t(uuid));
  }

  void set_predecessor(const size_t index, const uuid_t uuid, const endpoint_t endpoint) {
    routes[uuid] = endpoint;
    predecessors[index] = std::unique_ptr<uuid_t>(new uuid_t(uuid));
  }

  uuid_t successor() {
    return *successors[0];
  }

  uuid_t predecessor() {
    return *predecessors[0];
  }

  uuid_t closest_preceding_node(const uuid_t& uuid) {
    for( size_t m = BITS; m > 0; m-- ) {
      //TODO
      //if( self > uuid && id <= uuid ) {
      //}
    }
    return context->uuid;
  }

private:
  Context* context;
};
