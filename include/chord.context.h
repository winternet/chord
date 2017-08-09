#pragma once

#include <memory>

#include "chord.uuid.h"
#include "chord.types.h"

uuid_t generate_random();

struct Router;

struct Context {

  Context();

  //--- uuid
  uuid_t _uuid {generate_random()};
  //--- program options
  bool bootstrap { false };
  //--- promoted endpoint
  endpoint_t bind_addr { "0.0.0.0:50050" };
  endpoint_t join_addr { "0.0.0.0:50050" };

  //--- scheduling
  size_t stabilize_period_ms  { 10000 };
  size_t check_period_ms      { 10000 };

  uuid_t& uuid() { return _uuid; }

  Context set_uuid(const uuid_t& uuid);
  Context set_router(Router* router);

  private:
  Router* router { nullptr };
};
