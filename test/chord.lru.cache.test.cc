#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "chord.lru.cache.h"

using namespace std;
using namespace chord;

TEST(chord_lru_cache, constructor) {

  LRUCache<int, string> cache(5);
  ASSERT_TRUE(cache.empty());
  ASSERT_EQ(cache.size(), 0);
  ASSERT_EQ(cache.capacity(), 5);
}

TEST(chord_lru_cache, put) {

  LRUCache<int, string> cache(5);
  for(int i=0; i < 5; ++i) {
    ASSERT_FALSE(cache.exists(i));
    cache.put(i, std::to_string(i));
    ASSERT_TRUE(cache.exists(i));
  }
  ASSERT_FALSE(cache.empty());
  ASSERT_EQ(cache.size(), 5);

  cache.put(10, "latest");
  ASSERT_TRUE(cache.exists(10));
  ASSERT_EQ(cache.size(), 5);
  for(int i=1; i < 5; ++i)
    ASSERT_TRUE(cache.exists(i));
}

TEST(chord_lru_cache, erase) {

  LRUCache<int, string> cache(5);
  for(int i=0; i < 5; ++i) {
    ASSERT_FALSE(cache.exists(i));
    cache.put(i, std::to_string(i));
    ASSERT_TRUE(cache.exists(i));
    cache.erase(i);
  }
  ASSERT_TRUE(cache.empty());
  ASSERT_EQ(cache.size(), 0);
}

TEST(chord_lru_cache, get_touch) {

  LRUCache<int, string> cache(5);
  for(int i=0; i < 5; ++i) {
    cache.put(i, std::to_string(i));
    ASSERT_EQ(cache.get(i), std::to_string(i));
  }

  //refresh 0
  cache.get(0);
  cache.touch(1);
  //replace 2
  cache.put(1337, "replace 2");
  ASSERT_FALSE(cache.exists(2));

  try {
    cache.get(666);
    FAIL();
  } catch(const std::out_of_range& e) { }
}

