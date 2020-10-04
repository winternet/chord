#pragma once

#include <memory>
#include <atomic>
#include <future>
#include <grpc++/server.h>
#include "chord.context.h"
#include "chord.controller.service.h"
#include "chord.facade.h"
#include "chord.fs.facade.h"
#include "chord.shutdown.handler.h"
#include "chord.channel.pool.h"

namespace spdlog { class logger; }

namespace chord {

class Peer {
  static constexpr auto logger_name = "chord.peer";
private:

  std::atomic_flag stopped{false};
  std::promise<void> exit;

 protected:
  chord::Context context;

  std::unique_ptr<chord::ChannelPool> channel_pool;

  //--- chord facade
  std::unique_ptr<chord::ChordFacade> chord;

  //--- filesystem
  std::unique_ptr<chord::fs::Facade> filesystem;

  std::unique_ptr<chord::controller::Service> controller;

  std::unique_ptr<chord::ShutdownHandler> shutdown_handler;

  std::shared_ptr<spdlog::logger> logger;

  std::unique_ptr<grpc::Server> server;

  void start_server();

  Peer();

  void init();

 public:
  Peer(const Peer &) = delete;             // disable copying
  Peer &operator=(const Peer &) = delete;  // disable assignment

  explicit Peer(chord::Context context);
  virtual ~Peer();

  void start();
  void stop();
};

}  // namespace chord
