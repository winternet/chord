#pragma once

#include <grpc++/server_builder.h>
#include <memory>

#include "chord.client.h"
#include "chord.i.scheduler.h"
#include "chord.router.h"
#include "chord.service.h"
#include "chord.uuid.h"

namespace spdlog {
  class logger;
}
namespace chord {
//--- forward declarations
class AbstractScheduler;

class Client;
class Service;

struct Context;
struct Router;

class ChordFacade {
 public:
  ChordFacade(const ChordFacade &) = delete;             // disable copying
  ChordFacade &operator=(const ChordFacade &) = delete;  // disable assignment

  explicit ChordFacade(Context& context);

  void start();

  ::grpc::Service* grpc_service();

  /**
   * create new chord ring
   */
  void create();

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

 private:
  size_t next{0};

  chord::Context& context;

  std::unique_ptr<AbstractScheduler> scheduler;
  std::unique_ptr<chord::Router> router;

  //--- chord
  std::unique_ptr<chord::Client> client;
  std::unique_ptr<chord::Service> service;

  std::shared_ptr<spdlog::logger> logger;

  void start_scheduler();
};

}  // namespace chord
