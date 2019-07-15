#include "chord.peer.h"

#include <stdexcept>
#include <memory>
#include <thread>

#include <grpcpp/security/server_credentials.h>
#include <grpc++/server.h>
#include <grpc++/server_builder.h>
#include <grpc++/server_context.h>

#include "chord.controller.service.h"
#include "chord.fs.facade.h"
#include "chord.log.factory.h"
#include "chord.log.h"
#include "chord.node.h"
#include "chord.shutdown.handler.h"
#include "chord.uuid.h"

using grpc::ServerBuilder;
using namespace std;

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

  // initialize || join
  chord->start();

  server->Wait();
}

Peer::Peer(Context ctx)
    : context{ctx},
      chord{make_unique<chord::ChordFacade>(context)},
      filesystem{make_unique<chord::fs::Facade>(context, chord.get())},
      controller{make_unique<controller::Service>(context, filesystem.get())},
      logger{context.logging.factory().get_or_create(Peer::logger_name)}
{
  const auto fs = filesystem.get();
  chord->on_join().connect(fs, &chord::fs::Facade::on_join);
  chord->on_joined().connect(fs, &chord::fs::Facade::on_joined);
  chord->on_leave().connect(fs, &chord::fs::Facade::on_leave);
  chord->on_predecessor_fail().connect(fs, &chord::fs::Facade::on_predecessor_fail);
  chord->on_successor_fail().connect(fs, &chord::fs::Facade::on_successor_fail);
}

void Peer::start() {
  shutdown_handler = make_unique<chord::ShutdownHandler>(shared_from_this());
  logger->trace("peer with client-id {}", context.uuid());

  start_server();
  //--- blocks
}

void Peer::stop() {
  chord->stop();
}

} //namespace chord
