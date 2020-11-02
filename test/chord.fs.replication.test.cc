#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <iosfwd>
#include <string>

#include "chord.fs.replication.h"

using namespace std;
using namespace chord::fs;

auto Is(const std::uint32_t index, const std::uint32_t count) {
  using namespace testing;
  return AllOf(
      Field(&Replication::index, index),
      Field(&Replication::count, count));
}

TEST(Replication, ctor_default) {
  Replication repl;
  ASSERT_THAT(repl, Is(0,1));
}

TEST(Replication, ctor_count) {
  Replication repl(10);
  ASSERT_THAT(repl, Is(0,10));
}

TEST(Replication, ctor_index_count) {
  Replication repl(5, 10);
  ASSERT_THAT(repl, Is(5,10));
}

TEST(Replication, ctor_copy) {
  Replication repl(5,10);
  Replication cpy(repl);
  ASSERT_THAT(repl, Is(5,10));
}

TEST(Replication, operator_increment) {
  Replication repl(5,10);
  ASSERT_THAT(++repl, Is(6,10));
}

TEST(Replication, operator_post_increment) {
  Replication repl(5,10);
  const auto cpy = repl++;
  ASSERT_THAT(cpy, Is(5,10));
  ASSERT_THAT(repl, Is(6,10));
}

TEST(Replication, operator_decrement) {
  Replication repl(5,10);
  ASSERT_THAT(--repl, Is(4,10));
}

TEST(Replication, operator_post_decrement) {
  Replication repl(5,10);
  const auto cpy = repl--;
  ASSERT_THAT(cpy, Is(5,10));
  ASSERT_THAT(repl, Is(4,10));
}

TEST(Replication, operator_equals) {
  ASSERT_TRUE(Replication(4,5) == Replication(4,5));
  ASSERT_TRUE(Replication(5) == Replication(5));

  ASSERT_FALSE(Replication(1,4) == Replication(1,3));
  ASSERT_FALSE(Replication(1,4) == Replication(2,4));
}

TEST(Replication, operator_not_equals) {
  ASSERT_FALSE(Replication(4,5) != Replication(4,5));
  ASSERT_FALSE(Replication(5) != Replication(5));

  ASSERT_TRUE(Replication(1,4) != Replication(1,3));
  ASSERT_TRUE(Replication(1,4) != Replication(2,4));
}

TEST(Replication, operator_less) {
  ASSERT_FALSE(Replication(1,5) < Replication(0,5));
  ASSERT_FALSE(Replication(1,5) < Replication(0,2));

  ASSERT_TRUE(Replication(1,5) < Replication(0,7));
  ASSERT_TRUE(Replication(0,5) < Replication(1,7));
}

TEST(Replication, has_next) {
  ASSERT_TRUE(Replication(1,5).has_next());
  ASSERT_TRUE(Replication(3,5).has_next());
  ASSERT_FALSE(Replication(4,5).has_next());
  ASSERT_FALSE(Replication(5,5).has_next());
}

TEST(Replication, operator_bool) {
  ASSERT_TRUE(Replication(1,5));
  ASSERT_TRUE(Replication(3,5));
  ASSERT_TRUE(Replication(4,5));

  ASSERT_FALSE(Replication(5,5));
  ASSERT_FALSE(Replication(0,1));
}
