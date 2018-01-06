#pragma once

#include <grpc++/server_builder.h>
#include <memory>

#include "chord.client.h"
#include "chord.i.scheduler.h"
#include "chord.router.h"
#include "chord.service.h"
#include "chord.uuid.h"

namespace chord {
//--- forward declarations
class AbstractScheduler;

class Client;
class Service;

struct Context;
struct Router;

class ChordFacade {
 private:
  size_t next{0};

  chord::Context* _context;

  std::unique_ptr<AbstractScheduler> _scheduler;
  std::unique_ptr<chord::Router> _router;

  //--- chord
  std::unique_ptr<chord::Client> _client;
  std::unique_ptr<chord::Service> _service;

  void start_scheduler();

 public:
  ChordFacade(const ChordFacade &) = delete;             // disable copying
  ChordFacade &operator=(const ChordFacade &) = delete;  // disable assignment

  explicit ChordFacade(chord::Context* context);

  void start();

  ::grpc::Service* service();

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
};

}  // namespace chord
