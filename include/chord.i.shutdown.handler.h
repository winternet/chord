#pragma once

#include <atomic>
#include <boost/asio.hpp>
#include <functional>
#include <thread>

namespace chord {

class AbstractShutdownHandler {
 private:
  std::atomic<bool> is_signal_received;
  boost::asio::io_context io_context;
  std::thread signal_thread;
  boost::asio::signal_set signals;

  void handle_stop_internal(const boost::system::error_code &error, int signal_number) {
    is_signal_received = true;
    handle_stop(error, signal_number);
  }

  void initialize() {
    using std::placeholders::_1;
    using std::placeholders::_2;
    signals.async_wait(
        std::bind(&AbstractShutdownHandler::handle_stop_internal, this, _1, _2));
    signal_thread = std::thread(
        std::bind(static_cast<std::size_t (boost::asio::io_context::*)(void)>(
                      &boost::asio::io_context::run), &io_context));
  }

  virtual void handle_stop(const boost::system::error_code &error,
                                    int signal_number) = 0;

 public:
  AbstractShutdownHandler()
      : is_signal_received{false},
        io_context{},
        signals{io_context, SIGINT, SIGTERM, SIGQUIT} {
    initialize();
  }

  virtual ~AbstractShutdownHandler() {
    io_context.stop();
    signals.cancel();
    signal_thread.join();
  }

};

}  // namespace chord
