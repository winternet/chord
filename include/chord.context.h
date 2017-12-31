#pragma once

#include <memory>

#include "chord.path.h"
#include "chord.uuid.h"
#include "chord.types.h"

uuid_t generate_random();

namespace chord {
struct Router;
}

namespace chord {
struct Context {

  Context() = default;

  //--- uuid
  uuid_t _uuid{chord::uuid::random()};
  //--- program options
  bool bootstrap{false};
  bool no_controller{false};
  chord::path data_directory{"./data"};
  chord::path meta_directory{"./metadata"};
  chord::path config{"./config.yml"};
  //--- promoted endpoint
  endpoint_t bind_addr{"0.0.0.0:50050"};
  endpoint_t join_addr{"0.0.0.0:50050"};

  //--- scheduling
  size_t stabilize_period_ms{10000};
  size_t check_period_ms{10000};

  uuid_t &uuid() { return _uuid; }

  const uuid_t &uuid() const { return _uuid; }

  Context set_uuid(const uuid_t &uuid);

  Context set_router(chord::Router *router);

 private:
  Router *router {nullptr};
};
} //namespace chord
