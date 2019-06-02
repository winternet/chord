#pragma once

#include <grpc++/server_builder.h>
#include <memory>

#include "chord.signal.h"
#include "chord.client.h"
#include "chord.i.scheduler.h"
#include "chord.router.h"
#include "chord.service.h"
#include "chord.uuid.h"

namespace spdlog {
class logger;
}  // namespace spdlog

namespace chord {
//--- forward declarations
class AbstractScheduler;

class Client;
class Service;
class IClient;
class IService;

struct Context;
struct Router;

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

  std::unique_ptr<AbstractScheduler> scheduler;

  std::shared_ptr<spdlog::logger> logger;

  //--- events
  event_binary_t event_leave;

  void start_scheduler();
  void stop_scheduler();

 public:
  ChordFacade(const ChordFacade &) = delete;             // disable copying
  ChordFacade &operator=(const ChordFacade &) = delete;  // disable assignment

  explicit ChordFacade(Context& context);
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
  chord::common::RouterEntry successor(const uuid_t &uuid);

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

  event_binary_t& on_joined();

  event_binary_t& on_leave();

  event_unary_t& on_successor_fail();

  event_unary_t& on_predecessor_fail();

};

}  // namespace chord
