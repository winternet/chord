#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <thread>
#include "chord.concurrent.queue.h"

using namespace std;
using namespace chord;

class ConcurrentQueueTest : public ::testing::Test {
  protected:
    ConcurrentQueue<int> queue;
};

TEST_F(ConcurrentQueueTest, empty) {
  ASSERT_EQ(true, queue.empty());
  queue.push(42);
  ASSERT_EQ(false, queue.empty());
}

TEST_F(ConcurrentQueueTest, pop_blocks) {
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

