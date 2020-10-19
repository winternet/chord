#pragma once

#include <atomic>
#include <cassert>
#include <chrono>
#include <functional>
#include <future>
#include <memory>
#include <queue>
#include <sstream>
#include <thread>

#include "chord.i.scheduler.h"

namespace chord {

class Scheduler;
struct function_timer {
  private:
    using clock = std::chrono::system_clock;
    using time_point = clock::time_point;
    using duration = clock::duration;
  public:

  time_point  time;

  std::function<void()> func;

  function_timer() = default;

  function_timer(const time_point &t, std::function<void()> &&f)
      : time(t), func(f) {}

  bool operator<(const function_timer &rhs) const {
    return time < rhs.time;
  }

  // sort from smallest time to largest
  bool operator>(const function_timer &rhs) const {
    return time > rhs.time;
  }

  void operator()() {
    func();
  }

};

class Scheduler : public AbstractScheduler {
 private:
  using clock = std::chrono::system_clock;
  using time_point = clock::time_point;
  using duration = clock::duration;

  bool stop {false};
  bool task {false};
  std::condition_variable task_cv;
  std::condition_variable queue_cv;
  std::mutex mtx;

  std::vector<std::thread> threads;
  std::priority_queue<function_timer, std::vector<function_timer>, std::greater<function_timer> > tasks;

  void _shutdown() {
    // must be protected by mutex
    stop = true;
    queue_cv.notify_all();
    task_cv.notify_one();
  }


 public:
  Scheduler(const Scheduler &rhs) = delete;
  Scheduler &operator=(const Scheduler &rhs) = delete;

  Scheduler(size_t pool_size) {
    for(size_t i=0; i < pool_size; ++i) {
      threads.emplace_back([this]{ loop(); });
    }
  }

  Scheduler() : Scheduler(2) {
  }

  virtual ~Scheduler() {
    {
      std::unique_lock<std::mutex> lck(mtx);
      _shutdown();
    }
    for(auto& t : threads) t.join();
  }

  void shutdown() override {
    {
      std::unique_lock<std::mutex> lck(mtx);
      _shutdown();
    }
  }

  void schedule(const time_point &time, std::function<void()> &&func) override {
    schedule_intern(time, std::move(func));
  }

  void schedule(const duration &interval, std::function<void()> &&func) override {
    schedule_intern(interval, std::move(func));
  }

  //in format "%s %M %H %d %m %Y" "sec min hour date month year"
  void schedule(const std::string &time, std::function<void()> func) override {
    if (time.find('*')==std::string::npos && time.find('/')==std::string::npos) {
      std::tm tm = {};
      strptime(time.c_str(), "%s %M %H %d %m %Y", &tm);
      auto tp = clock::from_time_t(std::mktime(&tm));
      if (tp > clock::now())
        schedule_intern(tp, std::move(func));
    }
  }

 private:
  void schedule_intern(const time_point& time, std::function<void()> &&func) {
    std::unique_lock<std::mutex> lck(mtx);
    tasks.emplace(time, std::move(func));
    queue_cv.notify_one();
  }

  void schedule_intern(const duration& interval, std::function<void()> &&func) {
    std::unique_lock<std::mutex> lck(mtx);
    std::function<void()> wrapper = [this, interval, func]() mutable {
      func();
      this->schedule_intern(clock::now() + interval, std::move(func));
    };
    tasks.emplace(clock::now() + interval, std::move(wrapper));
    queue_cv.notify_one();
  }

  bool is_runnable() {
    if(stop) return true;
    if(tasks.empty()) return false;
    return tasks.top().time <= clock::now();
  }

  void loop() {
    using namespace std;

    //TODO: move to for loop init
    std::unique_lock<std::mutex> lck(mtx);
    while (!stop) {
      if(is_runnable()) {
        auto func = tasks.top();
        tasks.pop();

        if(this->task) task_cv.notify_one();
        else           queue_cv.notify_one();

        lck.unlock();
        func();
        lck.lock();
      } else if (!tasks.empty() && !task) {
        task = true;
        task_cv.wait_until(lck, tasks.top().time, [this]{ return is_runnable(); });
        task = false;
      } else {
        queue_cv.wait(lck);
      }
    }
    lck.unlock();
  }
};

} //namespace chord
