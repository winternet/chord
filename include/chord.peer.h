#pragma once

#include <grpc++/server_builder.h>
#include <memory>

#include "chord.facade.h"
#include "chord.fs.facade.h"
#include "chord.router.h"
#include "chord.uuid.h"

namespace chord {
class AbstractScheduler;
class Client;

class Service;

struct Context;
struct Router;
namespace controller {
class Service;
}  // namespace controller
}  // namespace chord

namespace chord {

class Peer {
 private:
  chord::Context context;

  //--- chord facade
  std::unique_ptr<chord::ChordFacade> chord;

  //--- filesystem
  std::unique_ptr<chord::fs::Facade> filesystem;

  std::unique_ptr<chord::controller::Service> controller;

  void start_server();

 public:
  Peer(const Peer &) = delete;             // disable copying
  Peer &operator=(const Peer &) = delete;  // disable assignment

  explicit Peer(chord::Context context);

  void start();
};

}  // namespace chord
