#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "chord.router.h"

#define ASSERT_NOT_NULL(pointer) ASSERT_TRUE(pointer)
#define ASSERT_NULL(pointer) ASSERT_FALSE(pointer)

#define EXPECT_NOT_NULL(pointer) EXPECT_TRUE(pointer)
#define EXPECT_NULL(pointer) EXPECT_FALSE(pointer)

using namespace std;
using namespace chord;

TEST(RouterTest, initialize) {
  Context context;
  Router router{context};

  ASSERT_NOT_NULL(router.successor());
  EXPECT_EQ(router.successor()->uuid, context.uuid());

  for (size_t i = 1; i < Router::BITS; i++) {
    ASSERT_NULL(router.successor(i));
  }

  for (size_t i = 0; i < Router::BITS; i++) {
    ASSERT_NULL(router.predecessor(i));
  }

  ASSERT_EQ(1, router.size());
}

TEST(RouterTest, dump) {
  auto context = Context{};
  Router router{context};
  std::cout << router;
}

TEST(RouterTest, closest_preceding_node) {
  Context context;
  context.set_uuid(0);
  Router router(context);

  for (int i = 0; i <= 100; i++) {
    router.set_successor(i, i, to_string(i));
  }

  auto predecessor = router.closest_preceding_node(200)->uuid;

  ASSERT_EQ(predecessor, 100);
  ASSERT_EQ(101, router.size());
}

TEST(RouterTest, closest_preceding_node_less_1) {
  Context context;
  context.set_uuid(1);
  Router router(context);

  router.set_successor(7, 100, to_string(100));

  // predecessor of 1 is 100
  uuid_t predecessor = router.closest_preceding_node(1)->uuid;

  ASSERT_EQ(predecessor, 100);
  ASSERT_EQ(2, router.size());
}

TEST(RouterTest, closest_preceding_node_mod) {
  Context context;
  context.set_uuid(999);
  Router router(context);

  // direct successor of 999 is 0
  for (int i = 0; i <= 100; i++) {
    router.set_successor(i, i, to_string(i));
  }

  uuid_t predecessor = router.closest_preceding_node(50)->uuid;

  ASSERT_EQ(predecessor, 49);
  ASSERT_EQ(102, router.size());
}

TEST(RouterTest, closest_preceding_node_mod_2) {
  Context context;
  context.set_uuid(5);
  Router router(context);

  // 5 -> [0]
  router.set_successor(0, 0, "0.0.0.0:50050");
  router.set_predecessor(0, 0, "0.0.0.0:50050");

  uuid_t predecessor = router.closest_preceding_node(5)->uuid;

  ASSERT_EQ(predecessor, 0);
  ASSERT_EQ(2, router.size());
}

/**
 * ring with 2 nodes
 *   - 0 @ 0.0.0.0:50050
 *   - 5 @ 0.0.0.0:50055
 * node 5 tries to find closest predecessor of id 2 -> 0
 */
TEST(RouterTest, set_uuid_resets_router) {
  Context context;
  Router router(context);
  auto uuid = context.uuid();

  auto successor = router.successor();
  ASSERT_EQ(successor->uuid, uuid);

  //--- calls router#reset() internally
  context.set_uuid(0);

  auto successor2 = router.successor();
  ASSERT_EQ(successor2->uuid, 0);
  ASSERT_EQ(successor2->endpoint, context.bind_addr);
  // first uuid + second uuid
  ASSERT_EQ(2, router.size());
}

TEST(RouterTest, closest_preceding_node_mod_3) {
  Context context;
  context.set_uuid(8);
  Router router(context);

  //  4<-8-[1]->4
  router.set_successor(0, 4, "0.0.0.0:50050");
  router.set_predecessor(0, 4, "0.0.0.0:50050");

  uuid_t predecessor = router.closest_preceding_node(1)->uuid;

  ASSERT_EQ(predecessor, 8);
  ASSERT_EQ(2, router.size());
}

TEST(RouterTest, closest_preceding_node_3) {
  Context context;
  context.set_uuid(8);
  Router router(context);

  // direct successor of 4<-8->10-[1]->4->...
  router.set_successor(0, 10, "0.0.0.0:50050");
  router.set_successor(1,  4, "0.0.0.0:50050");

  router.set_predecessor(0, 4, "0.0.0.0:50050");

  uuid_t predecessor = router.closest_preceding_node(1)->uuid;

  ASSERT_EQ(predecessor, 10);
  ASSERT_EQ(3, router.size());
}

TEST(RouterTest, set_successor_sets_preceding_nodes) {
  Context context;
  Router router{context};

  ASSERT_NOT_NULL(router.successor());
  EXPECT_EQ(router.successor()->uuid, context.uuid());

  // --> 0: {}, 1: {}, 2: {}, ...
  ASSERT_EQ(1, router.size());

  for (size_t i = 5; i < 10; i++) {
    router.set_successor(i, {1}, "1");
  }

  // --> 0: {1}, 1: {1}, 2: {1}, ..., 9:{1}, 10: {}
  ASSERT_EQ(2, router.size());
  for (size_t i = 0; i < 10; i++) {
    ASSERT_NOT_NULL(router.successor(i));
    ASSERT_EQ(router.successor(i)->uuid, uuid_t{1});
  }
}

TEST(RouterTest, set_successor_rewrites_same_preceding_nodes) {
  Context context;
  Router router{context};

  ASSERT_NOT_NULL(router.successor());
  EXPECT_EQ(router.successor()->uuid, context.uuid());

  // [0..5] -> uuid_t{5}
  router.set_successor(5, {5}, "5");
  // [6..10] -> uuid_t{5}
  router.set_successor(10, {10}, "10");

  for (size_t i = 0; i < 10; i++) {
    const auto succ = router.successor(i);
    ASSERT_NOT_NULL(succ);
    if(i <= 5) {
      ASSERT_EQ(router.successor(i)->uuid, uuid_t{5});
    } else {
      ASSERT_EQ(router.successor(i)->uuid, uuid_t{10});
    }
  }

  // insert new node
  router.set_successor(2, {2}, "2");

  for (size_t i = 0; i < 10; i++) {
    const auto succ = router.successor(i);
    ASSERT_NOT_NULL(succ);
    if(i <= 2) {
      ASSERT_EQ(router.successor(i)->uuid, uuid_t{2});
    } else if(i <= 5) {
      ASSERT_EQ(router.successor(i)->uuid, uuid_t{5});
    } else {
      ASSERT_EQ(router.successor(i)->uuid, uuid_t{10});
    }
  }
}
