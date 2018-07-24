#pragma once

#include <grpc++/server_builder.h>
#include <memory>

#include "chord.facade.h"
#include "chord.fs.facade.h"
#include "chord.router.h"
#include "chord.uuid.h"
#include "chord.shutdown.handler.h"

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
namespace spdlog {
  class logger;
}  // namespace spdlog

namespace chord {

class Peer : public std::enable_shared_from_this<Peer> {
  static constexpr auto logger_name = "chord.peer";

 private:
  chord::Context context;

  //--- chord facade
  std::unique_ptr<chord::ChordFacade> chord;

  //--- filesystem
  std::unique_ptr<chord::fs::Facade> filesystem;

  std::unique_ptr<chord::controller::Service> controller;

  std::unique_ptr<chord::ShutdownHandler> shutdown_handler;

  std::shared_ptr<spdlog::logger> logger;

  void start_server();

 public:
  Peer(const Peer &) = delete;             // disable copying
  Peer &operator=(const Peer &) = delete;  // disable assignment

  explicit Peer(chord::Context context);

  void start();
  void stop();
};

}  // namespace chord
