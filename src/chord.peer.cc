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

#define PEER_LOG(level) LOG(level) << "[peer] "

using grpc::ServerBuilder;
using namespace std;

using chord::common::RouterEntry;

namespace chord {

void Peer::start_server() {
  endpoint_t bind_addr = context->bind_addr;
  ServerBuilder builder;
  builder.AddListeningPort(bind_addr, grpc::InsecureServerCredentials());
  builder.RegisterService(chord->service());
  // controller service
  builder.RegisterService(controller.get());
  // filesystem service
  builder.RegisterService(filesystem->service());

  unique_ptr<grpc::Server> server(builder.BuildAndStart());
  PEER_LOG(debug) << "server listening on " << bind_addr;
  server->Wait();
}

Peer::Peer(shared_ptr<Context> context)
    : context{context},
      chord{make_unique<chord::ChordFacade>(context.get())},
      filesystem{make_unique<chord::fs::Facade>(context.get(), chord.get())},
      controller{make_unique<controller::Service>(filesystem.get())}
{
}

void Peer::start() {
  PEER_LOG(trace) << "peer with client-id " << context->uuid();

  chord->start();

  start_server();
  //--- blocks
}

} //namespace chord
