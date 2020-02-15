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
  struct sequence_tag {};
  struct key_tag {};
  struct value_tag {};


  struct change_node {
    change_node(const chord::node& node)
      : _node(node) {}
    void operator()(RouterEntry& entry) {
      entry._node = _node;
    }
  private:
    chord::node _node;
  };

  struct clear_node {
    clear_node(){}
    void operator()(RouterEntry& entry) {
      entry._node.reset();
    }
  };

  using entry_t = RouterEntry;
  using sequence_map_t = boost::multi_index::multi_index_container<
    entry_t,
    boost::multi_index::indexed_by<
        boost::multi_index::sequenced<boost::multi_index::tag<struct sequence_tag>>,
        boost::multi_index::ordered_non_unique<
          boost::multi_index::tag<struct key_tag>,
          boost::multi_index::member<entry_t, uuid, &entry_t::uuid>>>>;//,
        //boost::multi_index::ordered_non_unique<
        //  boost::multi_index::tag<struct value_tag>,
        //  boost::multi_index::member<entry_t, optional<endpoint>, &entry_t::_node>>>>;

  static constexpr auto logger_name = "chord.router";

  chord::Context &context;
  std::shared_ptr<spdlog::logger> logger;

  mutable mutex_t mtx;

  void init();
  void cleanup();

  //signal<void(const node)> event_predecessor_fail;
  //signal<void(const node)> event_predecessor_fail;

protected:
  sequence_map_t successors;
  RouterEntry _predecessor;

public:
  explicit Router(const chord::Router&) = delete;

  explicit Router(chord::Context &context);

  virtual ~Router();

  static uuid calc_successor_uuid_for_index(const uuid&, const size_t i);
  uuid calc_successor_uuid_for_index(const size_t i) const;

  void reset();

  bool has_successor() const;
  bool has_predecessor() const;

  void update(const std::set<chord::node>&);
  bool update(const node&);
  bool remove(const node&);
  bool remove(const uuid&);
  uuid get(const size_t) const;

  std::string print() const;
  std::ostream& print(std::ostream&) const;

  optional<node> successor() const;

  optional<node> predecessor() const;

  optional<node> closest_preceding_node(const uuid_t &uuid);

  std::set<node> finger() const;

  friend std::ostream &operator<<(std::ostream &os, const Router &router);

};
}  // namespace chord
