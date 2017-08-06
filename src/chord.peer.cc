#include <stdexcept>

#include "chord.peer.h"

#include "chord.i.scheduler.h"
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
using namespace std;

void ChordPeer::start_server() {
  endpoint_t bind_addr = context->bind_addr;
  ServerBuilder builder;
  builder.AddListeningPort(bind_addr, grpc::InsecureServerCredentials());
  builder.RegisterService(service.get());

  unique_ptr<grpc::Server> server(builder.BuildAndStart());
  PEER_LOG(debug) << "server listening on " << bind_addr;
  server->Wait();
}

ChordPeer::ChordPeer(shared_ptr<Context> context) 
  : scheduler { new Scheduler() }
  , context { context }
  , router  { new Router{context.get()} }
  , client  { new ChordClient{*context, *router} }
  , service { new ChordServiceImpl{*context, *router} }
{
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

ChordPeer::~ChordPeer() {}

void ChordPeer::start_scheduler() {
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
      next = (next % 8) + 1;
      //next = 0;
      fix_fingers(next);
      PEER_LOG(trace) << "[dump]" << *router;
      });
}

/**
 * join chord ring containing client-id.
 */
void ChordPeer::join() {
  client->join(context->join_addr);
}

/**
 * successor
 */
void ChordPeer::successor(const uuid_t& uuid) {
  service->successor(uuid);
}

/**
 * stabilize the ring
 */
void ChordPeer::stabilize() {
  client->stabilize();
}

/**
 * check predecessor
 */
void ChordPeer::check_predecessor() {
  client->check();
}

/**
 * fix finger table
 */
void ChordPeer::fix_fingers(size_t index) {
  //if( router->successor() == nullptr ) {
  //  PEER_LOG(trace) << "fixing fingers for direct successor.";
  //  next = 1;
  //}
  //size_t next = pow(2., (double)1-1);
  //uuid_t uuid = context->uuid() + next;
  //PEER_LOG(trace) << "fixing finger for " << to_string(uuid) << ".";
  //if( *router->successor() != context->uuid() )
  service->fix_fingers(index);
}

/**
 * create new chord ring
 */
void ChordPeer::create() {
  PEER_LOG(trace) << "bootstrapping new chord ring.";
  router->reset();
  //router->set_successor(
  //    0,
  //    context->uuid(),
  //    context->bind_addr);
}
