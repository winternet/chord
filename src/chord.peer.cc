#include "chord.peer.h"

#include <stdexcept>
#include <memory> 
#include <grpcpp/security/server_credentials.h>
#include <grpc++/server.h>
#include <grpc++/server_builder.h>
#include <grpc++/server_context.h>

#include <libfswatch/c++/monitor_factory.hpp>
#include <utility>
#include "chord.channel.pool.h"
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

  server = builder.BuildAndStart();
  logger->debug("server listening on {}, advertising {}", bind_addr, context.advertise_addr);

  // initialize || join
  chord->start();

  server->Wait();
  exit.set_value();
}

void Peer::init() {
  const auto fs = filesystem.get();
  chord->on_join().connect(&chord::fs::Facade::on_join, fs);
  chord->on_leave().connect(&chord::fs::Facade::on_leave, fs);
  chord->on_predecessor_update().connect(&chord::fs::Facade::on_predecessor_update, fs);
  chord->on_predecessor_fail().connect(&chord::fs::Facade::on_predecessor_fail, fs);
  chord->on_successor_fail().connect(&chord::fs::Facade::on_successor_fail, fs);
}

Peer::Peer()
      : logger{context.logging.factory().get_or_create(Peer::logger_name)}
{ }

Peer::Peer(Context ctx)
    : context{std::move(ctx)},
      channel_pool{make_unique<chord::ChannelPool>(context)},
      chord{make_unique<chord::ChordFacade>(context, channel_pool.get())},
      filesystem{make_unique<chord::fs::Facade>(context, chord.get(), channel_pool.get())},
      controller{make_unique<controller::Service>(context, filesystem.get())},
      logger{context.logging.factory().get_or_create(Peer::logger_name)}
{
  init();
}

Peer::~Peer() {
  logger->trace("[~]");
  stop();
}

void Peer::start() {
  if(context.register_shutdown_handler)
    shutdown_handler = make_unique<chord::ShutdownHandler>(this);
  logger->trace("peer with client-id {}", context.uuid());

  start_server();
  //--- blocks
}

void Peer::stop() {
  if(stopped.test_and_set()) {
    return;
  }
  chord->stop();
  if(server) {
    server->Shutdown();
    exit.get_future().wait();
  }
}

} //namespace chord
