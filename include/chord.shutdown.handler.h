#pragma once

#include <atomic>
#include <memory>

#include "chord.i.shutdown.handler.h"

namespace boost { namespace system { class error_code; } }
namespace chord { class Peer; }
namespace spdlog { class logger; }

namespace chord {

class ShutdownHandler: public AbstractShutdownHandler {
  static constexpr auto logger_name = "chord.shutdown.handler";

  static std::atomic<int> invocation_cnt;

  private:
    std::shared_ptr<Peer> peer;
    std::shared_ptr<spdlog::logger> logger;

  public:
   ShutdownHandler();
   ShutdownHandler(std::shared_ptr<Peer>);

  private:
   void handle_stop(const boost::system::error_code& error, int signal);
};

}  // namespace chord

