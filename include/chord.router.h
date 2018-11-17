#pragma once
#include <array>
#include <mutex>
#include <map>

#include "chord.context.h"

namespace spdlog {
  class logger;
}

namespace chord {

struct Router {

  static constexpr size_t BITS = 256;
  static constexpr auto logger_name = "chord.router";

  explicit Router(const chord::Router&) = delete;

  explicit Router(chord::Context &context);

  virtual ~Router();

  void cleanup();

  void reset();

  bool has_successor() const;

  /**
   * get the amount of known routes.
   *
   * note that this does _neither_ return the size of the
   * chord ring nor the currently known routes, it just returns
   * all routes it once knew.
   */
  size_t size() const;

  optional<node> successor(size_t idx) const;

  optional<node> predecessor(size_t idx) const;

  void set_successor(const size_t index, const chord::node& node);

  void set_predecessor(const size_t index, const chord::node& node);

  void reset(const uuid_t& uuid);

  void reset(const node& n);

  optional<node> successor();

  const optional<node> predecessor() const;

  optional<node> predecessor();

  optional<node> closest_preceding_node(const uuid_t &uuid);

  friend std::ostream &operator<<(std::ostream &os, const Router &router);

 private:
  using mutex_t = std::recursive_mutex;
  chord::Context &context;
  std::shared_ptr<spdlog::logger> logger;

  mutable std::recursive_mutex mtx;
  std::map<uuid_t, endpoint_t> routes;

  std::array<optional<uuid_t>, BITS> predecessors;
  std::array<optional<uuid_t>, BITS> successors;
};
}  // namespace chord
