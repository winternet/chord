#pragma once

#include <memory>

#include "chord.uuid.h"
#include "chord.types.h"

uuid_t generate_random();

class Router;

struct Context {

  Context();

  //--- uuid
  uuid_t uuid {generate_random()};
  //--- program options
  bool bootstrap { false };
  //--- promoted endpoint
  endpoint_t bind_addr { "0.0.0.0:50050" };
  endpoint_t join_addr { "0.0.0.0:50050" };

  size_t stabilize_period_ms { 10000 };

  Router* router { nullptr };

};
