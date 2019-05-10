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

//void Peer::setup_root_logger() {
//  std::vector<spdlog::sink_ptr> sinks;
//  sinks.push_back(std::make_shared<spdlog::sinks::rotating_file_sink_mt>("logs/chord.log", 1024*1024*20, 3));
//  auto chord_root_logger = std::make_shared<spdlog::logger>("chord.root", begin(sinks), end(sinks));
//  spdlog::register_logger(chord_root_logger);
//}
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
      logger{context.logging.factory().get_or_create(Peer::logger_name)}
{
  const auto fs = filesystem.get();
  chord->on_join().connect(fs, &chord::fs::Facade::on_join);
  chord->on_leave().connect(fs, &chord::fs::Facade::on_leave);
  chord->on_predecessor_fail().connect(fs, &chord::fs::Facade::on_predecessor_fail);
  chord->on_successor_fail().connect(fs, &chord::fs::Facade::on_successor_fail);
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
