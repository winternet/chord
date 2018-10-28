#include <stdexcept>
#include <memory>
#include <thread>

#include <grpc++/server.h>
#include <grpc++/server_builder.h>
#include <grpc++/server_context.h>

#include "chord.i.scheduler.h"
#include "chord.scheduler.h"
#include "chord.router.h"
#include "chord.peer.h"
#include "chord.fs.facade.h"

#include "chord.controller.service.h"

using grpc::ServerBuilder;
using namespace std;

using chord::common::RouterEntry;

namespace chord {

void Peer::start_server() {
  const auto bind_addr = context.bind_addr;
  ServerBuilder builder;
  builder.AddListeningPort(bind_addr, grpc::InsecureServerCredentials());
  builder.RegisterService(chord->grpc_service());
  // controller service
  builder.RegisterService(controller.get());
  // filesystem service
  builder.RegisterService(filesystem->grpc_service());

  unique_ptr<grpc::Server> server(builder.BuildAndStart());
  logger->debug("server listening on {}", bind_addr);
  server->Wait();
}

Peer::Peer(Context ctx)
    : context{ctx},
      chord{make_unique<chord::ChordFacade>(context)},
      filesystem{make_unique<chord::fs::Facade>(context, chord.get())},
      controller{make_unique<controller::Service>(context, filesystem.get())},
      logger{log::get_or_create(logger_name)}
{
  chord->set_take_callback(filesystem->take_consumer_callback());
  chord->set_take_callback(filesystem->take_producer_callback());

  chord->set_on_leave_callback(filesystem->on_leave_callback());
}

void Peer::start() {
  shutdown_handler = make_unique<chord::ShutdownHandler>(shared_from_this());
  logger->trace("peer with client-id {}", context.uuid());

  chord->start();

  start_server();
  //--- blocks
}

void Peer::stop() {
  chord->stop();
}

} //namespace chord
