#pragma once

#include <map>
#include <string>
#include <boost/array.hpp>

#include "chord.uuid.h"

typedef std::string endpoint_t;

#define BITS 256

struct Router {

  Router(const uuid_t& self)
    : self { self }
  {}

  uuid_t self;

  std::map<uuid_t, endpoint_t> routes;
  boost::array<uuid_t, BITS> finger;

  std::unique_ptr<uuid_t> _successor   { nullptr };
  std::unique_ptr<uuid_t> _predecessor { nullptr };

  endpoint_t get(const size_t& index) {
    return routes[finger[index]];
  }

  endpoint_t get(const uuid_t& uuid) {
    return routes[uuid];
  }

  void add(const size_t index, const uuid_t uuid, const endpoint_t endpoint) {
    routes[uuid] = endpoint;
    finger[index] = uuid;
  }

  void set_successor(const uuid_t uuid, const endpoint_t endpoint) {
    _successor = std::unique_ptr<uuid_t>(new uuid_t(uuid));
    routes[uuid] = endpoint;
  }

  uuid_t successor() {
    return *_successor;
  }

  void set_predecessor(const uuid_t uuid, const endpoint_t endpoint) {
    _predecessor = std::unique_ptr<uuid_t>(new uuid_t(uuid));
    routes[uuid] = endpoint;
  }

  uuid_t predecessor() {
    return *_predecessor;
  }

  uuid_t closest_preceding_node(const uuid_t& uuid) {
    for( size_t m = BITS; m > 0; m-- ) {
      //TODO
      //if( self > uuid && id <= uuid ) {
      //}
    }
  }
};
