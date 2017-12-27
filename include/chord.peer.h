#pragma once

#include <memory>
#include <grpc++/server_builder.h>

#include "chord.client.h"
#include "chord.service.h"
#include "chord.i.scheduler.h"

//#include "chord.queue.h"
#include "chord.uuid.h"
#include "chord.router.h"

class AbstractScheduler;

namespace chord {
class Client;

class Service;

struct Context;
struct Router;
namespace controller {
class Service;
}
namespace fs {
class Service;

class Client;
}
}

namespace chord {

class Peer : std::enable_shared_from_this<Peer> {
 private:
  size_t next{0};

  std::unique_ptr<AbstractScheduler> scheduler{nullptr};
  const std::shared_ptr<chord::Context> &context;

  std::unique_ptr<chord::Router> router;//       { nullptr };

  //--- chord
  std::unique_ptr<chord::Client> client;//  { nullptr };
  std::unique_ptr<chord::Service> service; // { nullptr };

  //--- filesystem
  std::shared_ptr<chord::fs::Client> fs_client;// { nullptr };
  std::shared_ptr<chord::fs::Service> fs_service;// { nullptr };

  std::unique_ptr<chord::controller::Service> controller;// { nullptr };

  void start_server();

  void start_scheduler();

 public:
  Peer(const Peer &) = delete;             // disable copying
  Peer &operator=(const Peer &) = delete;  // disable assignment

  explicit Peer(std::shared_ptr<chord::Context> context);

  void start();

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

} //namespace chord
