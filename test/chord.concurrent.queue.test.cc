#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <thread>
#include "chord.concurrent.queue.h"

using namespace std;
using namespace chord;

TEST(chord_concurrent_queue, empty) {
  ConcurrentQueue<int> queue;
  ASSERT_EQ(true, queue.empty());
  queue.push(42);
  ASSERT_EQ(false, queue.empty());
}

TEST(chord_concurrent_queue, pop_blocks) {
  ConcurrentQueue<int> queue;
  thread consumer([&]{
      std::cerr << "consumer thread blocks";
      ASSERT_EQ(42,queue.pop());
  });
  ASSERT_EQ(true, queue.empty());
  queue.push(42);
  // let consumer pop value
  consumer.join();

  ASSERT_EQ(true, queue.empty());
}

