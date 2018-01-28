#include <stdexcept>
#include <memory>
#include <thread>

#include <grpc++/server.h>
#include <grpc++/server_builder.h>
#include <grpc++/server_context.h>

#include "chord.facade.h"

#include "chord.i.scheduler.h"
#include "chord.scheduler.h"
#include "chord.router.h"

#define CHORD_FACADE_LOG(level) LOG(level) << "[chord-facade] "

using grpc::ServerBuilder;
using namespace std;

using chord::common::RouterEntry;

namespace chord {

ChordFacade::ChordFacade(Context& ctx)
    : context{ctx},
      scheduler{make_unique<Scheduler>()},
      router{make_unique<Router>(context)},
      client{make_unique<Client>(context, router.get())},
      service{make_unique<Service>(context, router.get())} {}

::grpc::Service* ChordFacade::grpc_service() {
  return service.get();
}

void ChordFacade::start() {
  CHORD_FACADE_LOG(trace) << "peer with client-id " << context.uuid();

  if (context.bootstrap) {
    create();
  } else {
    join();
  }

  start_scheduler();
}

void ChordFacade::start_scheduler() {
  //--- stabilize
  scheduler->schedule(chrono::milliseconds(context.stabilize_period_ms), [this] {
    stabilize();
    //CHORD_FACADE_LOG(trace) << "[dump]" << *router;
  });

  //--- check predecessor
  scheduler->schedule(chrono::milliseconds(context.check_period_ms), [this] {
    check_predecessor();
    //CHORD_FACADE_LOG(trace) << "[dump]" << *router;
  });

  //--- fix fingers
  scheduler->schedule(chrono::milliseconds(context.check_period_ms), [this] {
    //TODO(christoph) use uuid::UUID_BITS_MAX
    next = (next%8) + 1;
    //next = 0;
    CHORD_FACADE_LOG(trace) << "fix fingers with next index next: " << next;
    fix_fingers(next);
    CHORD_FACADE_LOG(trace) << "[dump]" << *router;
  });
}

/**
 * join chord ring containing client-id.
 */
void ChordFacade::join() {
  client->join(context.join_addr);
}

/**
 * successor
 */
RouterEntry ChordFacade::successor(const uuid_t &uuid) {
  return service->successor(uuid);
}

/**
 * stabilize the ring
 */
void ChordFacade::stabilize() {
  client->stabilize();
}

/**
 * check predecessor
 */
void ChordFacade::check_predecessor() {
  client->check();
}

/**
 * fix finger table
 */
void ChordFacade::fix_fingers(size_t index) {
  service->fix_fingers(index);
}

/**
 * create new chord ring
 */
void ChordFacade::create() {
  CHORD_FACADE_LOG(trace) << "bootstrapping new chord ring.";
  router->reset();
}

} //namespace chord