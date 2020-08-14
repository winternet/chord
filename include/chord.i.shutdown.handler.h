#pragma once

#include <atomic>
#include <iostream>
#include <boost/asio.hpp>
#include <functional>
#include <thread>

namespace chord {

class AbstractShutdownHandler {
 private:
  std::atomic<bool> is_signal_received;
  boost::asio::io_service io_service;
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
        std::bind(static_cast<std::size_t (boost::asio::io_service::*)(void)>(
                      &boost::asio::io_service::run), &io_service));
  }

  virtual void handle_stop(const boost::system::error_code &error,
                                    int signal_number) = 0;

 public:
  AbstractShutdownHandler()
      : is_signal_received{false},
        io_service{},
        signals{io_service, SIGINT, SIGTERM, SIGQUIT} {
    initialize();
  }

  virtual ~AbstractShutdownHandler() {
    io_service.stop();
    signals.cancel();
    signal_thread.join();
  }

};

}  // namespace chord
