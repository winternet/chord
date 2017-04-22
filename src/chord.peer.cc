#include <stdexcept>

#include "chord.scheduler.h"
#include "chord.router.h"
#include "chord.client.h"
#include "chord.service.h"

#include <grpc++/server.h>
#include <grpc++/server_builder.h>
#include <grpc++/server_context.h>

#include "chord.context.h"

#define PEER_LOG(level) LOG(level) << "[peer] "

using grpc::ServerBuilder;

class ChordPeer {
private:
  size_t next { 0 };
  Scheduler scheduler;
  std::shared_ptr<Context> context { nullptr };
  std::shared_ptr<Router> router { nullptr };
  std::unique_ptr<ChordClient> client { nullptr };
  std::unique_ptr<ChordServiceImpl> service { nullptr };

  void start_server() {
    endpoint_t bind_addr = context->bind_addr;
    ServerBuilder builder;
    builder.AddListeningPort(bind_addr, grpc::InsecureServerCredentials());
    builder.RegisterService(service.get());
  
    std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
    PEER_LOG(debug) << "server listening on " << bind_addr;
    server->Wait();
  }

public:
  ChordPeer(std::shared_ptr<Context> context) 
    : context { context }
    , router  { context->router }
    , client  { new ChordClient{context} }
    , service { new ChordServiceImpl{context} }
  {
    PEER_LOG(trace) << "peer with client-id " << context->uuid();
    if( context->bootstrap ) {
      create();
    } else {
      join();
    }
    configure_scheduler();
    start_server();
    //--- blocks
  }

  virtual ~ChordPeer() {}

  void configure_scheduler() {
    //--- stabilize
    scheduler.schedule(std::chrono::milliseconds(context->stabilize_period_ms), [this] {
      stabilize();
      //PEER_LOG(trace) << "[dump]" << *router;
    });

    //--- check predecessor
    scheduler.schedule(std::chrono::milliseconds(context->check_period_ms), [this] {
      check_predecessor();
      //PEER_LOG(trace) << "[dump]" << *router;
    });

    //--- fix fingers
    scheduler.schedule(std::chrono::milliseconds(context->check_period_ms), [this] {
      next = (next % 8) + 1;
      fix_fingers(next);
      PEER_LOG(trace) << "[dump]" << *router;
    });
  }

  /**
   * join chord ring containing client-id.
   */
  void join() {
    client->join(context->join_addr);
  }

  /**
   * successor
   */
  void successor(const uuid_t& uuid) {
    service->successor(uuid);
  }

  /**
   * stabilize the ring
   */
  void stabilize() {
    client->stabilize();
  }

  /**
   * check predecessor
   */
  void check_predecessor() {
     client->check();
  }

  /**
   * fix finger table
   */
  void fix_fingers(size_t index) {
    //if( router->successor() == nullptr ) {
    //  PEER_LOG(trace) << "fixing fingers for direct successor.";
    //  next = 1;
    //}
    //size_t next = std::pow(2., (double)1-1);
    //uuid_t uuid = context->uuid() + next;
    //PEER_LOG(trace) << "fixing finger for " << to_string(uuid) << ".";
    service->fix_fingers(index);
  }

  /**
   * create new chord ring
   */
  void create() {
    PEER_LOG(trace) << "bootstrapping new chord ring.";
    router->reset();
    router->set_successor(
        0,
        context->uuid(),
        context->bind_addr);
  }
};

