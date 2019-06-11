#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "chord.node.h"
#include "chord.types.h"
#include "chord.uuid.h"

using namespace std;

using chord::node;

TEST(chord_node, equals) {
  const auto node0 = node{"1234","127.0.0.1:8080"};
  const auto node1 = node{"1234","127.0.0.1:8080"};
  ASSERT_EQ(node0, node1);
}

TEST(chord_node, string) {
  const auto n = node{"1234","127.0.0.1:8080"};
  ASSERT_EQ(n.string(), "1234@127.0.0.1:8080");
}

TEST(chord_node, output_node) {
  const auto n = node{"1234","127.0.0.1:8080"};
  std::stringstream ss;
  ss << n;
  ASSERT_EQ(ss.str(), "1234@127.0.0.1:8080");
}
