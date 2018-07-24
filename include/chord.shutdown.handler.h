#pragma once

#include "chord.i.shutdown.handler.h"
#include "chord.log.h"
#include "chord.peer.h"
#include <memory>

namespace spdlog {
  class logger;
}  // namespace spdlog

namespace chord {

class ShutdownHandler: public AbstractShutdownHandler {
  static constexpr auto logger_name = "chord.shutdown.handler";

  private:
    std::shared_ptr<Peer> peer;
    std::atomic<int> counter{0};
    std::shared_ptr<spdlog::logger> logger{log::get_or_create(logger_name)};

  public:
   ShutdownHandler(std::shared_ptr<Peer>);

  private:
   void handle_stop(const boost::system::error_code& error, int signal);
};

}  // namespace chord

