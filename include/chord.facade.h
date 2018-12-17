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

struct Context;
struct Router;

class ChordFacade {
  static constexpr auto logger_name = "chord.facade";

 public:
  ChordFacade(const ChordFacade &) = delete;             // disable copying
  ChordFacade &operator=(const ChordFacade &) = delete;  // disable assignment

  explicit ChordFacade(Context& context);
  explicit ChordFacade(Context& ctx, Router* router, Client* client, Service* service);

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
   * nth direct successor
   */
  chord::common::RouterEntry nth_successor(const uuid_t &uuid, const size_t n);

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

  auto& on_join() {
    return event_joined;
  }

  auto& on_leave() {
    return service->on_leave();
  }

 private:
  size_t next{0};

  chord::Context& context;

  std::unique_ptr<chord::Router> router;

  //--- chord
  std::unique_ptr<chord::Client> client;
  std::unique_ptr<chord::Service> service;

  std::unique_ptr<AbstractScheduler> scheduler;

  std::shared_ptr<spdlog::logger> logger;

  //--- events
  signal<void(const node, const node)> event_joined;

  void start_scheduler();
  void stop_scheduler();
};

}  // namespace chord
