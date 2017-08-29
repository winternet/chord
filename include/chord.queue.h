#pragma once

#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>

namespace chord {
  template <typename T>
  class queue
  {
    private:
      std::queue<T> queue_;
      std::mutex mutex;
      std::condition_variable condition;
    public:
      queue()=default;
      queue(const queue&) = delete;            // disable copying
      queue& operator=(const queue&) = delete; // disable assignment

      virtual ~queue() {}

      T pop() {
        std::unique_lock<std::mutex> lock(mutex);
        while (queue_.empty())
        {
          condition.wait(lock);
        }
        auto val = queue_.front();
        queue_.pop();
        return val;
      }

      void pop(T& item) {
        std::unique_lock<std::mutex> lock(mutex);
        while (queue_.empty())
        {
          condition.wait(lock);
        }
        item = queue_.front();
        queue_.pop();
      }

      void push(const T& item) {
        std::unique_lock<std::mutex> lock(mutex);
        queue_.push(item);
        lock.unlock();
        condition.notify_one();
      }
  };
};
