#include "chord.facade.h"

#include <memory>

#include <grpcpp/server_context.h>

#include "chord.client.h"
#include "chord.common.h"
#include "chord.context.h"
#include "chord.i.scheduler.h"
#include "chord.log.factory.h"
#include "chord.log.h"
#include "chord.router.h"
#include "chord.scheduler.h"
#include "chord.service.h"
#include "chord.types.h"
#include "chord.utils.h"

using namespace std;

using chord::common::make_node;

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
  //TODO commont this in after tests
  //leave();
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
    next = (next + 1) % Router::BITS;
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
  //scheduler->shutdown();
  // inform successor and predecessor about leave
  client->leave();

  // NOTE dont use the setter because we must not reset the router
  context._uuid = (router->predecessor().value_or(context.node()).uuid);
  const auto successor_node = router->successor().value_or(context.node());
  const auto predecessor_node = router->predecessor().value_or(context.node());
  if(successor_node != predecessor_node)
    event_leave(predecessor_node, successor_node);
}

void ChordFacade::join_failed(const grpc::Status& status) {
    logger->warn("failed to notify {}; error: {}", context.join_addr, utils::to_string(status));
    router->reset();
}

/**
 * join chord ring containing client-id.
 */
void ChordFacade::join() {
  const auto status = client->join(context.join_addr);
  if(!status.ok()) {
    logger->warn("[join] failed to join {}; error: {}", context.join_addr, utils::to_string(status));
    return;
  }

  logger->info("[join] successfully joined ring via {} - notifying predecessor {}.", context.join_addr, router->predecessor()->endpoint);
  const auto status_pred = client->notify(*router->predecessor(), *router->successor(), context.node());
  logger->info("[join] successfully joined ring via {} - notifying successor {}.", context.join_addr, router->successor()->endpoint);
  const auto status_succ = client->notify(*router->successor(), *router->predecessor(), context.node());
  if(!status_pred.ok() || !status_succ.ok()) {
    return join_failed(status_pred.ok() ? status_succ : status_pred);
  }

  event_join(*router->successor());
}

/**
 * successor
 */
chord::node ChordFacade::successor(const uuid_t &uuid) {
  return make_node(service->lookup(uuid));
}

chord::node ChordFacade::successor() {
  return make_node(service->lookup(context.uuid()));
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

ChordFacade::event_unary_t& ChordFacade::on_join() {
  return event_join;
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
