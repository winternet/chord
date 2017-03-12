#pragma once

#include <memory>

#include "chord.uuid.h"
#include "chord.router.h"

uuid_t generate_random();

struct Context {

  //--- program options
  bool bootstrap { false };
  //--- promoted endpoint
  endpoint_t bind_addr { "0.0.0.0:50050" };
  endpoint_t join_addr { "0.0.0.0:50050" };

  uuid_t uuid {generate_random()};


  std::shared_ptr<Router> router { new Router{uuid} };

  //std::unique_ptr<uuid_t> predecessor { nullptr };
  //std::unique_ptr<uuid_t> successor { nullptr };

};
