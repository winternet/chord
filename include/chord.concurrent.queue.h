#pragma once

#include <condition_variable>
#include <mutex>
#include <queue>

namespace chord {
template<typename T>
class ConcurrentQueue {
  private:
    using mutex_t = std::mutex;
    using lock_guard_t = std::unique_lock<mutex_t>;
    std::queue<T> q;
    mutable std::mutex mtx;
    std::condition_variable cond;
  public:
    void push(const T& val) {
      lock_guard_t lock(mtx);
      q.push(val);
      lock.unlock();
      cond.notify_one();
    }

    bool empty() const {
      lock_guard_t lock(mtx);
      return q.empty();
    }

    // we dont care whether the ctor throws -> return by value
    // http://www.gotw.ca/gotw/008.htm
    T pop() {
      lock_guard_t lock(mtx);
      while(q.empty()) cond.wait(lock);
      const T& val = q.front();
      q.pop();
      return val;
    }

};
} //namespace chord
