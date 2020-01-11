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

private:
  struct sequence_tag {};
  struct key_tag {};
  struct value_tag {};

  struct RouterEntry {
    chord::uuid uuid;
    optional<chord::endpoint> endpoint;
    bool valid() const { return endpoint.has_value(); }
    chord::node node() const { return {uuid, *endpoint}; }
  };

  struct change_endpoint {
    change_endpoint(const chord::endpoint& endpoint)
      : endpoint(endpoint) {}
    void operator()(RouterEntry& entry) {
      entry.endpoint = endpoint;
    }
  private:
    chord::endpoint endpoint;
  };

  using entry_t = RouterEntry;
  using sequence_map_t = boost::multi_index::multi_index_container<
    entry_t,
    boost::multi_index::indexed_by<
        boost::multi_index::sequenced<boost::multi_index::tag<struct sequence_tag>>,
        boost::multi_index::ordered_non_unique<
          boost::multi_index::tag<struct key_tag>,
          boost::multi_index::member<entry_t, uuid, &entry_t::uuid>>,
        boost::multi_index::ordered_non_unique<
          boost::multi_index::tag<struct value_tag>,
          boost::multi_index::member<entry_t, optional<endpoint>, &entry_t::endpoint>>>>;

private:
  static constexpr auto logger_name = "chord.router";

  chord::Context &context;
  std::shared_ptr<spdlog::logger> logger;

  mutable mutex_t mtx;

  sequence_map_t successors;
  RouterEntry _predecessor;

  void init();
  void cleanup();

public:
  explicit Router(const chord::Router&) = delete;

  explicit Router(chord::Context &context);

  virtual ~Router();

  static uuid calc_successor_uuid_for_index(const uuid&, const size_t i);
  uuid calc_successor_uuid_for_index(const size_t i) const;

  //static uuid calc_predecessor_uuid_for_index(const uuid&, const size_t i);
  //uuid calc_predecessor_uuid_for_index(const size_t i) const;

  void reset();

  bool has_successor() const;
  bool has_predecessor() const;

  void update(const std::set<chord::node>&);
  bool update(const node&);
  bool remove(const node&);
  uuid get(const size_t) const;

  /**
   * get the amount of known routes.
   *
   * note that this does _neither_ return the size of the
   * chord ring nor the currently known routes, it just returns
   * all routes it once knew.
   */
  //size_t size() const;

  //optional<node> successor(size_t idx) const;

  //optional<node> predecessor(size_t idx) const;

  //void replace_successor(const chord::node& old_node, const chord::node& new_node);

  //void set_successor(const size_t index, const chord::node& node);

  //void replace_predecessor(const chord::node& old_node, const chord::node& new_node);

  ////TODO:  update predecessors accordingly?
  //bool update_successor(const chord::node& old_node, const chord::node& new_node);

  //void set_predecessor(const size_t index, const chord::node& node);

  //void reset(const uuid_t& uuid);

  //void reset(const node& n);

  optional<node> successor() const;

  //void fill_predecessors(const chord::node&);
  //void fill_successors(const chord::node&);

  //const optional<node> predecessor() const;

  optional<node> predecessor() const;

  optional<node> closest_preceding_node(const uuid_t &uuid);

  friend std::ostream &operator<<(std::ostream &os, const Router &router);

};
}  // namespace chord
