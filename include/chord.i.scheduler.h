#pragma once

#include <functional>
#include <string>
#include <chrono>
#include <future>
#include <queue>
#include <thread>
#include <memory>
#include <sstream>
#include <cassert>

using std::string;
using std::function;

class AbstractScheduler {
 public:
  virtual void schedule(const std::chrono::system_clock::time_point &time, function<void()> &&func) = 0;

  virtual void schedule(const std::chrono::system_clock::duration &interval, function<void()> func) = 0;

  //in format "%s %M %H %d %m %Y" "sec min hour date month year"
  virtual void schedule(const string &time, function<void()> func) = 0;

  virtual ~AbstractScheduler() = default;
};
