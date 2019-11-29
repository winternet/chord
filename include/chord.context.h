#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>

#include "chord.log.h"
#include "chord.node.h"
#include "chord.path.h"
#include "chord.types.h"
#include "chord.uuid.h"

uuid_t generate_random();

namespace chord { struct Router; }

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
  chord::endpoint bind_addr{"0.0.0.0:50050"};
  chord::endpoint join_addr; //{"0.0.0.0:50050"};

  //--- scheduling
  size_t stabilize_period_ms{10000};
  size_t check_period_ms{10000};

  //--- replication / striping
  std::uint32_t replication_cnt{1}; //default replication factor if not overriden

  //--- logging
  log::Logging logging;

  inline const uuid_t &uuid() const { return _uuid; }
  inline const chord::node node() const { return {_uuid, bind_addr}; }

  void set_uuid(const uuid_t uuid);

  Context set_router(chord::Router *router);

  /**
   * Validate the context or throw exceptions for invalid contexts
   */
  static void validate(const Context& context);

 private:
  Router *router {nullptr};
};
} //namespace chord
