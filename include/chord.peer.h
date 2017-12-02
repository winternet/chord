#pragma once

#include "chord.router.h"
#include "chord.client.h"
#include "chord.service.h"
#include "chord.i.scheduler.h"
#include <grpc++/server_builder.h>
#include "chord.queue.h"
#include "chord.uuid.h"
#include "chord.fs.service.h"


struct Context;
struct Router;
class AbstractScheduler;

//using grpc::ServerBuilder;
namespace chord {

  class Peer {
    private:
      size_t next { 0 };
      chord::queue<std::string> commands;

      std::unique_ptr<AbstractScheduler> scheduler { nullptr };
      const std::shared_ptr<Context>& context;

      std::unique_ptr<Router> router        { nullptr };
      std::unique_ptr<chord::Client> client   { nullptr };
      std::unique_ptr<chord::Service> service { nullptr };
      std::unique_ptr<chord::fs::Service> fs_service { nullptr };

      void start_server();
      void start_scheduler();

    public:
      Peer(const Peer&) = delete;             // disable copying
      Peer& operator=(const Peer&) = delete;  // disable assignment

      Peer(const std::shared_ptr<Context>& context);

      virtual ~Peer();

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
      chord::common::RouterEntry successor(const uuid_t& uuid);

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
