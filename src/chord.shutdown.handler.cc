#include "chord.shutdown.handler.h"

#include <atomic>
#include <cstdlib>
#include <memory>

#include "chord.log.h"
#include "chord.peer.h"

namespace boost { namespace system { class error_code; } }
namespace spdlog { class logger; }

namespace chord {

std::atomic<int> ShutdownHandler::invocation_cnt {0};

ShutdownHandler::ShutdownHandler() : ShutdownHandler(nullptr) {}

ShutdownHandler::ShutdownHandler(std::shared_ptr<Peer> peer) 
  : peer{peer}
  , logger{log::get_or_create(logger_name)}
{}

void ShutdownHandler::handle_stop([[maybe_unused]] const boost::system::error_code& error, [[maybe_unused]] int signal_number) {

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
  logger->info("stopped peer - exiting.");
}

}  // namespace chord

