#pragma once

#include <functional>
#include <chrono>
#include <future>
#include <queue>
#include <thread>
#include <memory>
#include <sstream>
#include <assert.h>

#include "chord.i.scheduler.h"

struct function_timer {
  std::function<void()> func;
  std::chrono::system_clock::time_point time;

  function_timer() {}

  function_timer(std::function<void()> &&f, const std::chrono::system_clock::time_point &t)
      : func(f), time(t) {}

  //Note: we want our priority_queue to be ordered in terms of
  //smallest time to largest, hence the > in operator<. This isn't good
  //practice - it should be a separate struct -  but I've done this for brevity.
  bool operator<(const function_timer &rhs) const {
    return time > rhs.time;
  }

  void get() {
    func();
  }

  void operator()() {
    func();
  }

};

class Scheduler : public AbstractScheduler {
 private:
  bool shutdown;
  std::unique_ptr<std::thread> thread;
  std::priority_queue<function_timer> tasks;

  Scheduler &operator=(const Scheduler &rhs) = delete;

  Scheduler(const Scheduler &rhs) = delete;

 public:
  Scheduler()
      : shutdown(false),
        thread(new std::thread([this] {
                                 //--- setup time
                                 using namespace std::chrono_literals;
                                 std::this_thread::sleep_for(2s);
                                 //--- start loop
                                 loop();
                               }
        )) {}

  virtual ~Scheduler() {
    shutdown = true;
    thread->join();
  }

  virtual void schedule(const std::chrono::system_clock::time_point &time, std::function<void()> &&func) override {
    std::function<void()> threadFunc = [func]() {
      std::thread t(func);
      t.detach();
    };
    tasks.push(function_timer(std::move(threadFunc), time));
  }

  virtual void schedule(const std::chrono::system_clock::duration &interval, std::function<void()> func) override {
    std::function<void()> threadFunc = [func]() {
      std::thread t(func);
      t.detach();
    };
    this->scheduleIntern(interval, threadFunc);
  }

  //in format "%s %M %H %d %m %Y" "sec min hour date month year"
  virtual void schedule(const std::string &time, std::function<void()> func) override {
    if (time.find("*")==std::string::npos && time.find("/")==std::string::npos) {
      std::tm tm = {};
      strptime(time.c_str(), "%s %M %H %d %m %Y", &tm);
      auto tp = std::chrono::system_clock::from_time_t(std::mktime(&tm));
      if (tp > std::chrono::system_clock::now())
        scheduleIntern(tp, std::move(func));
    }
  }

 private:
  void scheduleIntern(const std::chrono::system_clock::time_point &time, std::function<void()> &&func) {
    tasks.push(function_timer(std::move(func), time));
  }

  void scheduleIntern(std::chrono::system_clock::duration interval, std::function<void()> func) {
    std::function<void()> waitFunc = [this, interval, func]() {
      func();
      this->scheduleIntern(interval, func);
    };
    scheduleIntern(std::chrono::system_clock::now() + interval, std::move(waitFunc));
  }

  void loop() {
    while (!shutdown) {
      auto now = std::chrono::system_clock::now();
      while (!tasks.empty() && tasks.top().time <= now) {
        function_timer f = tasks.top();
        f.get();
        tasks.pop();
      }

      if (tasks.empty()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
      } else {
        std::this_thread::sleep_for(tasks.top().time - std::chrono::system_clock::now());
      }
    }
  }
};
