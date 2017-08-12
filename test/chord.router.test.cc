#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <string>

#include "chord.router.h"

#define ASSERT_NOT_NULL(pointer) ASSERT_TRUE(pointer != nullptr)
#define ASSERT_NULL(pointer) ASSERT_TRUE(pointer == nullptr)

#define EXPECT_NOT_NULL(pointer) EXPECT_TRUE(pointer != nullptr)
#define EXPECT_NULL(pointer) EXPECT_TRUE(pointer == nullptr)

using namespace std;

TEST(RouterTest, initialize) {
  Context context;
  Router router(&context);

  ASSERT_NOT_NULL(router.successor());
  EXPECT_EQ(*router.successor(), context.uuid());

  for(int i=1; i < BITS; i++) {
    ASSERT_NULL(router.successors[i]);
  }

  for(int i=0; i < BITS; i++ ) {
    ASSERT_NULL(router.predecessors[i]);
  }
}

TEST(RouterTest, closest_preceding_node) {
  Context context;
  context.set_uuid(0);
  Router router(&context);

  for(int i=0; i <= 100; i++ ) {
    router.set_successor(i, i, to_string(i));
  }

  uuid_t predecessor = router.closest_preceding_node(200);

  ASSERT_EQ(predecessor, 100);
}

TEST(RouterTest, closest_preceding_node_less_1) {
  Context context;
  context.set_uuid(1);
  Router router(&context);

  router.set_successor(7, 100, to_string(100));

  // predecessor of 1 is 100
  uuid_t predecessor = router.closest_preceding_node(1);

  ASSERT_EQ(predecessor, 100);
}

TEST(RouterTest, closest_preceding_node_mod) {
  Context context;
  context.set_uuid(999);
  Router router(&context);

  // direct successor of 999 is 0
  for(int i=0; i <= 100; i++ ) {
    router.set_successor(i, i, to_string(i));
  }

  uuid_t predecessor = router.closest_preceding_node(50);

  ASSERT_EQ(predecessor, 49);
}

TEST(RouterTest, closest_preceding_node_mod_2) {
  Context context;
  context.set_uuid(5);
  Router router(&context);

  // direct successor of 5 is 0
  router.set_successor(0, 0, "0.0.0.0:50050");
  router.set_predecessor(0, 0, "0.0.0.0:50050");

  uuid_t predecessor = router.closest_preceding_node(5);

  ASSERT_EQ(predecessor, 0);
}

/**
 * ring with 2 nodes
 *   - 0 @ 0.0.0.0:50050
 *   - 5 @ 0.0.0.0:50055
 * node 5 tries to find closest predecessor of id 2 -> 0
 */
TEST(RouterTest, set_uuid_resets_router ) {
  Context context;
  Router router(&context);
  auto uuid = context.uuid();

  auto successor = router.successor();
  ASSERT_EQ(*successor, uuid);

  //--- calls router#reset() internally
  context.set_uuid(0);

  successor = router.successor();
  ASSERT_EQ(*successor, 0);
  ASSERT_EQ(router.get(uuid_t(0)), context.bind_addr);
}
