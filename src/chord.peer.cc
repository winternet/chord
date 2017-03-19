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
  Scheduler scheduler;
  std::shared_ptr<Router> router { nullptr };
  std::shared_ptr<Context> context { nullptr };
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
    scheduler.schedule(std::chrono::milliseconds(context->stabilize_period_ms), [this] {
      auto now = std::chrono::system_clock::now();
      auto tt = std::chrono::system_clock::to_time_t(now);
      stabilize();
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
   * stabilize the ring
   */
  void stabilize() {
    client->stabilize();
  }

  /**
   * find successor
   */

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

