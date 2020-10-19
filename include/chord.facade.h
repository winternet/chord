#pragma once

#include <memory>

#include "chord.i.scheduler.h"
#include "chord.router.h"
#include "chord.uuid.h"

#include "chord.i.client.h"
#include "chord.i.service.h"
#include "chord.node.h"
#include "chord.signal.h"

namespace chord { struct Context; }
namespace chord { class ChannelPool; }
namespace grpc { class Service; }
namespace spdlog { class logger; }  // lines 14-14


namespace chord {

class ChordFacade {
  static constexpr auto logger_name = "chord.facade";
public:
  using event_binary_t = signal<void(const node, const node)>;
  using event_unary_t = signal<void(const node)>;

 private:
  size_t next{0};

  chord::Context& context;

  std::unique_ptr<chord::Router> router;

  //--- chord
  std::unique_ptr<chord::IClient> client;
  std::unique_ptr<chord::IService> service;

  std::shared_ptr<AbstractScheduler> scheduler;

  std::shared_ptr<spdlog::logger> logger;

  //--- events
  event_binary_t event_leave;
  event_unary_t event_join;

  void start_scheduler();
  void stop_scheduler();
  void join_failed(const grpc::Status&);

 public:
  ChordFacade(const ChordFacade &) = delete;             // disable copying
  ChordFacade &operator=(const ChordFacade &) = delete;  // disable assignment

  explicit ChordFacade(Context& context, ChannelPool* channel_pool);
  explicit ChordFacade(Context& ctx, Router* router, IClient* client, IService* service);

  void start();

  void stop();

  ::grpc::Service* grpc_service();

  /**
   * create new chord ring
   */
  void create();

  /**
   * leave chord ring containing.
   */
  void leave();

  /**
   * join chord ring containing client-id.
   */
  void join();

  /**
   * successor
   */
  chord::node successor(const uuid_t &uuid);
  chord::node successor();

  /**
   * stabilize the ring
   */
  void stabilize();

  /**
   * check predecessor
   */
  void check_predecessor();

  /**
   * fix finger table
   */
  void fix_fingers(size_t index);

  event_unary_t& on_join();

  event_binary_t& on_leave();

  event_binary_t& on_predecessor_update();

  event_unary_t& on_successor_fail();

  event_unary_t& on_predecessor_fail();

};

}  // namespace chord
