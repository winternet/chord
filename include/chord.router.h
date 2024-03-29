#pragma once
#include <cstddef>
#include <array>
#include <iosfwd>
#include <map>
#include <set>
#include <memory>
#include <mutex>
#include <optional>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/multi_index/ordered_index.hpp>

#include "chord.signal.h"
#include "chord.types.h"
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
    std::optional<chord::node> _node;
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

  signal<const node> event_successor_fail;
  signal<const node> event_predecessor_fail;

  signal<const node, const node> event_successor_update;
  signal<const node, const node> event_predecessor_update;


protected:
  sequence_map_t successors;
  std::optional<node> _predecessor;

public:
  explicit Router(const chord::Router&) = delete;

  explicit Router(chord::Context &context) noexcept;

  virtual ~Router() noexcept;

  static uuid calc_successor_uuid_for_index(const uuid&, const size_t i);
  uuid calc_successor_uuid_for_index(const size_t i) const;

  void reset();

  bool has_successor() const;
  bool has_predecessor() const;

  inline signal<const node>& on_successor_fail() { return event_successor_fail; }
  inline signal<const node>& on_predecessor_fail() { return event_predecessor_fail; }
  inline signal<const node, const node>& on_predecessor_update() { return event_predecessor_update; }

  void update(const std::set<chord::node>&);
  bool update(const node&);
  bool remove(const node&, const bool signal=true);
  bool remove(const uuid&, const bool signal=true);

  uuid get(const size_t) const;
  std::set<node> get() const;

  std::string print() const;
  std::ostream& print(std::ostream&) const;

  std::optional<node> successor() const;
  node successor_or_self() const;

  std::optional<node> predecessor() const;

  std::vector<node> closest_preceding_nodes(const uuid& uuid);
  std::optional<node> closest_preceding_node(const uuid& uuid);

  friend std::ostream &operator<<(std::ostream &os, const Router &router);

};
}  // namespace chord

template<> struct fmt::formatter<chord::Router> : ostream_formatter {};
