#include "chord.i.shutdown.handler.h"
#include "chord.log.h"
#include "chord.peer.h"
#include <memory>
#include <cstdlib>

namespace spdlog {
  class logger;
}  // namespace spdlog

namespace chord {

std::atomic<int> ShutdownHandler::invocation_cnt {0};

ShutdownHandler::ShutdownHandler() : peer{nullptr} {}

ShutdownHandler::ShutdownHandler(std::shared_ptr<Peer> peer) : peer{peer} {}

void ShutdownHandler::handle_stop(const boost::system::error_code& error, int signal_number) {

  if(invocation_cnt.fetch_add(1) > 0) {
    logger->warn("received force quit");
    std::abort();
  }

  if(!peer) {
    logger->warn("cleanup not possible since no peer set - exiting");
    std::exit(0);
  }

  logger->info("shutdown handler is taking care of graceful leave - press Ctrl + C to force quit.");
  
  // register a new shutdown handler that 
  // will be invoked during graceful leave
  ShutdownHandler force;

  peer->stop();
  std::exit(0);
}

}  // namespace chord

