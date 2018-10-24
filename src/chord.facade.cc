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

using grpc::ServerBuilder;
using namespace std;

using chord::common::RouterEntry;

namespace chord {

ChordFacade::ChordFacade(Context& ctx)
    : context{ctx},
      scheduler{make_unique<Scheduler>()},
      router{make_unique<Router>(context)},
      client{make_unique<Client>(context, router.get())},
      service{make_unique<Service>(context, router.get(), client.get())},
      logger{log::get_or_create(logger_name)}
      {}

::grpc::Service* ChordFacade::grpc_service() {
  return service.get();
}

void ChordFacade::stop() {
  logger->trace("shutting down scheduler...");
  stop_scheduler();
  logger->trace("shutting down chord...");
  leave();
}

void ChordFacade::stop_scheduler() {
  scheduler->shutdown();
}

void ChordFacade::start() {
  logger->trace("peer with client-id {}", context.uuid());

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
  });

  //--- check predecessor
  scheduler->schedule(chrono::milliseconds(context.check_period_ms), [this] {
    check_predecessor();
  });

  //--- fix fingers
  scheduler->schedule(chrono::milliseconds(context.check_period_ms), [this] {
    //TODO use uuid::UUID_BITS_MAX
    next = (next%8) + 1;
    //next = 0;
    logger->trace("fix fingers with next index next: {}", next);
    fix_fingers(next);
    logger->trace("[dump] {}", *router);
  });
}

/**
 * leave chord ring
 */
void ChordFacade::leave() {
  client->leave();
}
/**
 * join chord ring containing client-id.
 */
void ChordFacade::join() {
  client->join(context.join_addr);
  client->stabilize();
  client->take();
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
  logger->trace("bootstrapping new chord ring.");
  router->reset();
}

} //namespace chord
