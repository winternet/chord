#include <stdexcept>
#include <memory>
#include <thread>

#include <grpc++/server.h>
#include <grpc++/server_builder.h>
#include <grpc++/server_context.h>

#include "chord.peer.h"

#include "chord.i.scheduler.h"
#include "chord.scheduler.h"
#include "chord.router.h"
#include "chord.client.h"
#include "chord.service.h"
#include "chord.fs.service.h"

#include "chord.context.h"

#define PEER_LOG(level) LOG(level) << "[peer] "

using grpc::ServerBuilder;
using namespace std;

using chord::common::RouterEntry;

namespace chord {

  void Peer::start_server() {
    endpoint_t bind_addr = context->bind_addr;
    ServerBuilder builder;
    builder.AddListeningPort(bind_addr, grpc::InsecureServerCredentials());
    builder.RegisterService(service.get());
    // filesystem service
    builder.RegisterService(fs_service.get());

    unique_ptr<grpc::Server> server(builder.BuildAndStart());
    PEER_LOG(debug) << "server listening on " << bind_addr;
    server->Wait();
  }

  Peer::Peer(const shared_ptr<Context>& context) 
    : scheduler { new Scheduler() }
  , context { context }
  , router  { make_unique<Router>(context.get()) }
  , client  { make_unique<chord::Client>(*context, *router) }
  , service { make_unique<chord::Service>(*context, *router) }
  , fs_service { make_unique<chord::fs::Service>(*context) }
  {
  }

  void Peer::start() {
    PEER_LOG(trace) << "peer with client-id " << context->uuid();

    if( context->bootstrap ) {
      create();
    } else {
      join();
    }

    start_scheduler();
    start_server();
    //--- blocks
  }

  Peer::~Peer() {}

  void Peer::start_scheduler() {
    //--- stabilize
    scheduler->schedule(chrono::milliseconds(context->stabilize_period_ms), [this] {
        stabilize();
        //PEER_LOG(trace) << "[dump]" << *router;
        });

    //--- check predecessor
    scheduler->schedule(chrono::milliseconds(context->check_period_ms), [this] {
        check_predecessor();
        //PEER_LOG(trace) << "[dump]" << *router;
        });

    //--- fix fingers
    scheduler->schedule(chrono::milliseconds(context->check_period_ms), [this] {
        //TODO(christoph) use uuid::UUID_BITS_MAX
        next = (next % 8) + 1;
        //next = 0;
        PEER_LOG(trace) << "fix fingers with next index next: " << next;
        fix_fingers(next);
        PEER_LOG(trace) << "[dump]" << *router;
        });
  }

  /**
   * join chord ring containing client-id.
   */
  void Peer::join() {
    client->join(context->join_addr);
  }

  /**
   * successor
   */
  RouterEntry Peer::successor(const uuid_t& uuid) {
    return service->successor(uuid);
  }

  /**
   * stabilize the ring
   */
  void Peer::stabilize() {
    client->stabilize();
  }

  /**
   * check predecessor
   */
  void Peer::check_predecessor() {
    client->check();
  }

  /**
   * fix finger table
   */
  void Peer::fix_fingers(size_t index) {
    service->fix_fingers(index);
  }

  /**
   * create new chord ring
   */
  void Peer::create() {
    PEER_LOG(trace) << "bootstrapping new chord ring.";
    router->reset();
  }

} //namespace chord
