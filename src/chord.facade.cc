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

ChordFacade::ChordFacade(Context* context)
    : _context{context},
      _scheduler{make_unique<Scheduler>()},
      _router{make_unique<Router>(_context)},
      _client{make_unique<Client>(_context, _router.get())},
      _service{make_unique<Service>(_context, _router.get())} {}

::grpc::Service* ChordFacade::service() {
  return _service.get();
}

void ChordFacade::start() {
  CHORD_FACADE_LOG(trace) << "peer with client-id " << _context->uuid();

  if (_context->bootstrap) {
    create();
  } else {
    join();
  }

  start_scheduler();
}

void ChordFacade::start_scheduler() {
  //--- stabilize
  _scheduler->schedule(chrono::milliseconds(_context->stabilize_period_ms), [this] {
    stabilize();
    //CHORD_FACADE_LOG(trace) << "[dump]" << *_router;
  });

  //--- check predecessor
  _scheduler->schedule(chrono::milliseconds(_context->check_period_ms), [this] {
    check_predecessor();
    //CHORD_FACADE_LOG(trace) << "[dump]" << *_router;
  });

  //--- fix fingers
  _scheduler->schedule(chrono::milliseconds(_context->check_period_ms), [this] {
    //TODO(christoph) use uuid::UUID_BITS_MAX
    next = (next%8) + 1;
    //next = 0;
    CHORD_FACADE_LOG(trace) << "fix fingers with next index next: " << next;
    fix_fingers(next);
    CHORD_FACADE_LOG(trace) << "[dump]" << *_router;
  });
}

/**
 * join chord ring containing client-id.
 */
void ChordFacade::join() {
  _client->join(_context->join_addr);
}

/**
 * successor
 */
RouterEntry ChordFacade::successor(const uuid_t &uuid) {
  return _service->successor(uuid);
}

/**
 * stabilize the ring
 */
void ChordFacade::stabilize() {
  _client->stabilize();
}

/**
 * check predecessor
 */
void ChordFacade::check_predecessor() {
  _client->check();
}

/**
 * fix finger table
 */
void ChordFacade::fix_fingers(size_t index) {
  _service->fix_fingers(index);
}

/**
 * create new chord ring
 */
void ChordFacade::create() {
  CHORD_FACADE_LOG(trace) << "bootstrapping new chord ring.";
  _router->reset();
}

} //namespace chord
