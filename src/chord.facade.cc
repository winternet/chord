#include <stdexcept>
#include <memory>
#include <thread>

#include <grpc++/server.h>
#include <grpc++/server_builder.h>
#include <grpc++/server_context.h>

#include "chord.facade.h"

#include "chord.log.h"
#include "chord.i.scheduler.h"
#include "chord.scheduler.h"
#include "chord.router.h"

using grpc::ServerBuilder;
using namespace std;

using chord::common::RouterEntry;

namespace chord {

ChordFacade::ChordFacade(Context& ctx)
    : context{ctx},
      router{make_unique<Router>(context)},
      client{make_unique<Client>(context, router.get())},
      service{make_unique<Service>(context, router.get(), client.get())},
      scheduler{make_unique<Scheduler>()},
      logger{ctx.logging.factory().get_or_create(logger_name)}
      {}

/**
 * Used for testing purposes.
 */
ChordFacade::ChordFacade(Context& ctx, Router* router, IClient* client, IService* service)
    : context{ctx},
      router{router},
      client{client},
      service{service},
      scheduler{make_unique<Scheduler>()},
      logger{ctx.logging.factory().get_or_create(logger_name)}
      {}

::grpc::Service* ChordFacade::grpc_service() {
  return service->grpc_service();
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
  // freeze state: do no longer participate on node updates
  scheduler->shutdown();
  // inform successor and predecessor about leave
  client->leave();

  const auto successor_node = router->successor().value_or(context.node());
  const auto predecessor_node = router->predecessor().value_or(context.node());
  event_leave(predecessor_node, successor_node);
}
/**
 * join chord ring containing client-id.
 */
void ChordFacade::join() {
  const auto success = client->join(context.join_addr);
  if(!success) {
    logger->warn("failed to join {}", context.join_addr);
    return;
  }

  logger->info("successfully joined {}", context.join_addr);
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
  // lost connection and we know where to join -> re-join
  if(!router->has_successor() && !context.join_addr.empty()) {
    join();
  }
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

ChordFacade::event_binary_t& ChordFacade::on_joined() {
  return service->on_joined();
}

ChordFacade::event_binary_t& ChordFacade::on_leave() {
  return event_leave;
}

ChordFacade::event_unary_t& ChordFacade::on_successor_fail() {
  return client->on_successor_fail();
}

ChordFacade::event_unary_t& ChordFacade::on_predecessor_fail() {
  return client->on_predecessor_fail();
}
/**
 * create new chord ring
 */
void ChordFacade::create() {
  logger->trace("bootstrapping new chord ring.");
  router->reset();
}

} //namespace chord
