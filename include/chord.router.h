#pragma once
#include <cstddef>
#include <array>
#include <iosfwd>
#include <map>
#include <set>
#include <memory>
#include <mutex>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/multi_index/ordered_index.hpp>

#include "chord.signal.h"
#include "chord.types.h"
#include "chord.optional.h"
#include "chord.node.h"
#include "chord.uuid.h"

namespace chord { struct Context; }

namespace spdlog {
  class logger;
}

namespace chord {

struct Router {
public:
  static constexpr size_t BITS = 256;
  using mutex_t = std::recursive_mutex;

protected:
  struct RouterEntry {
    chord::uuid uuid;
    optional<chord::node> _node;
    inline bool valid() const { return _node.has_value(); }
    inline chord::node node() const { return *_node; }
    inline std::ostream& print(std::ostream& os) const {
      os << (valid() ? _node->string() : "<empty>");
      return os;
    }
  };

private:
  static constexpr auto logger_name = "chord.router";

  using entry_t = RouterEntry;
  using sequence_map_t = std::array<RouterEntry, Router::BITS>;

  chord::Context &context;
  std::shared_ptr<spdlog::logger> logger;

  mutable mutex_t mtx;

  void init();
  void cleanup();

  signal<void(const node)> event_successor_fail;
  signal<void(const node)> event_predecessor_fail;

  signal<void(const node, const node)> event_successor_update;
  signal<void(const node, const node)> event_predecessor_update;


protected:
  sequence_map_t successors;
  optional<node> _predecessor;

public:
  explicit Router(const chord::Router&) = delete;

  explicit Router(chord::Context &context);

  virtual ~Router();

  static uuid calc_successor_uuid_for_index(const uuid&, const size_t i);
  uuid calc_successor_uuid_for_index(const size_t i) const;

  void reset();

  bool has_successor() const;
  bool has_predecessor() const;

  inline signal<void(const node)>& on_successor_fail() { return event_successor_fail; }
  inline signal<void(const node)>& on_predecessor_fail() { return event_predecessor_fail; }
  inline signal<void(const node, const node)>& on_predecessor_update() { return event_predecessor_update; }

  void update(const std::set<chord::node>&);
  bool update(const node&);
  bool remove(const node&);
  bool remove(const uuid&);

  uuid get(const size_t) const;
  std::set<node> get() const;

  std::string print() const;
  std::ostream& print(std::ostream&) const;

  optional<node> successor() const;
  node successor_or_self() const;

  optional<node> predecessor() const;

  std::vector<node> closest_preceding_nodes(const uuid& uuid);
  optional<node> closest_preceding_node(const uuid& uuid);

  friend std::ostream &operator<<(std::ostream &os, const Router &router);

};
}  // namespace chord
