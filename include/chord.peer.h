#pragma once

#include <memory>
#include <grpc++/server_builder.h>

#include "chord.uuid.h"
#include "chord.router.h"
#include "chord.facade.h"
#include "chord.fs.facade.h"


namespace chord {
class AbstractScheduler;
class Client;

class Service;

struct Context;
struct Router;
namespace controller {
class Service;
}
}

namespace chord {

class Peer {
 private:
  std::shared_ptr<chord::Context> context;

  //--- chord facade
  std::unique_ptr<chord::ChordFacade> chord;

  //--- filesystem
  std::unique_ptr<chord::fs::Facade> filesystem;


  std::unique_ptr<chord::controller::Service> controller;

  void start_server();

 public:
  Peer(const Peer &) = delete;             // disable copying
  Peer &operator=(const Peer &) = delete;  // disable assignment

  explicit Peer(std::shared_ptr<chord::Context> context);

  void start();

};

} //namespace chord
