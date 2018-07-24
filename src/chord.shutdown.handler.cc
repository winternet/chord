#include "chord.i.shutdown.handler.h"
#include "chord.log.h"
#include "chord.peer.h"
#include <memory>

namespace spdlog {
  class logger;
}  // namespace spdlog

namespace chord {

ShutdownHandler::ShutdownHandler(std::shared_ptr<Peer> peer) : peer{peer} {}

void ShutdownHandler::handle_stop(const boost::system::error_code& error, int signal_number) {
  logger->info("shutdown handler is taking care of graceful leave");
  peer->stop();
  exit(0);
}

}  // namespace chord

