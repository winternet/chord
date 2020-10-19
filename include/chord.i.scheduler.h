#pragma once

#include <cassert>
#include <chrono>
#include <functional>
#include <future>
#include <memory>
#include <queue>
#include <sstream>
#include <string>
#include <thread>

namespace chord {

class AbstractScheduler {
 public:
  virtual void schedule(const std::chrono::system_clock::time_point&, std::function<void()> &&) = 0;

  virtual void schedule(const std::chrono::system_clock::duration&, std::function<void()> &&) = 0;

  //in format "%s %M %H %d %m %Y" "sec min hour date month year"
  virtual void schedule(const std::string &time, std::function<void()> func) = 0;

  virtual void shutdown() = 0;

  virtual ~AbstractScheduler() = default;
};

} //namespace chord
