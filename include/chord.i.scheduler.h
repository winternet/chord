#pragma once

#include <functional>
#include <chrono>
#include <future>
#include <queue>
#include <thread>
#include <memory>
#include <sstream>
#include <assert.h>

class AbstractScheduler
{
  public:
    virtual void schedule(const std::chrono::system_clock::time_point& time, std::function<void()>&& func) = 0;

    virtual void schedule(const std::chrono::system_clock::duration& interval, std::function<void()> func) = 0;

    //in format "%s %M %H %d %m %Y" "sec min hour date month year"
    virtual void schedule(const std::string &time, std::function<void()> func) = 0;

    virtual ~AbstractScheduler() {}
};
