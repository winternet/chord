#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <cstddef>
#include <iostream>
#include <string>
#include "chord.context.h"
#include "util/chord.test.helper.h"
#include "chord.node.h"
#include "chord.path.h"
#include "chord.router.spy.h"
#include "chord.signal.h"
#include "chord.uuid.h"

#define ASSERT_NOT_NULL(pointer) ASSERT_TRUE(pointer)
#define ASSERT_NULL(pointer) ASSERT_FALSE(pointer)

#define EXPECT_NOT_NULL(pointer) EXPECT_TRUE(pointer)
#define EXPECT_NULL(pointer) EXPECT_FALSE(pointer)

using namespace std;
using namespace chord;
using namespace chord::test;

TEST(RouterTest, initialize) {
  Context context = make_context(0);
  RouterSpy router{context};

  //TODO check
  //ASSERT_FALSE(router.successor());
  ASSERT_FALSE(router.predecessor());

  ASSERT_FALSE(router.has_successor());
  ASSERT_FALSE(router.has_predecessor());

  for (size_t i = 1; i < Router::BITS; i++) {
    const auto entry = router.entry(i);
    ASSERT_FALSE(entry.valid());
    ASSERT_EQ(entry.uuid, chord::Router::calc_successor_uuid_for_index(context.uuid(), i));
  }
  ASSERT_EQ(context.node(), router.closest_preceding_node(9999));
  ASSERT_FALSE(router.closest_preceding_nodes(9999).empty());
}

TEST(RouterTest, dump) {
  auto context = Context{};
  Router router{context};
  std::cout << router;
}

TEST(RouterTest, closest_preceding_node) {
  Context context = make_context(0);
  RouterSpy router(context);

  ASSERT_FALSE(router.has_predecessor());
  ASSERT_FALSE(router.has_successor());

  for (size_t i = 1; i <= 100; i++) {
    router.update(node{i, "127.0.0.1:50"+to_string(i)});
  }
  router.update(node{199, "192.168.1.1:50199"});
  //note: 2^i-1
  router.print(std::cerr);
  ASSERT_EQ(router.closest_preceding_nodes(200).size(), 9);

  const std::vector<size_t> expected_nodes{0,2,3,5,9,17,33,65,199};

  for(size_t i=Router::BITS; i > 0; --i) {
    const auto predecessors = router.closest_preceding_nodes(i);

    const bool is_sorted = std::is_sorted(predecessors.crbegin(), predecessors.crend());

    const bool all_between = 
      std::all_of(predecessors.begin(), predecessors.end(), [&](const auto& pred) {
        return uuid::between(context.uuid(), pred.uuid, uuid{i}) || pred == context.node() ;
    });

    const auto all_expected_nodes =
      std::all_of(predecessors.begin(), predecessors.end(), [&](const auto& p) {
          const bool pred_contained = std::any_of(expected_nodes.begin(), expected_nodes.end(), [&](const auto& nbr) {
              return uuid{nbr} == p.uuid;
              });
          if(!pred_contained) std::cerr << "\n\n\nNOT CONTAINED: " << p;
          return pred_contained;
          });

    ASSERT_TRUE(is_sorted);
    ASSERT_TRUE(all_between);
    ASSERT_TRUE(all_expected_nodes);
  }

  ASSERT_TRUE(router.has_predecessor());
  ASSERT_EQ(router.predecessor()->uuid, 199);
  ASSERT_TRUE(router.has_successor());

  auto predecessor = router.closest_preceding_node(200);
  ASSERT_EQ(predecessor->uuid, 199);
  ASSERT_EQ(predecessor->endpoint, "192.168.1.1:50199");

  predecessor = router.closest_preceding_node(257);
  ASSERT_EQ(predecessor->uuid, 199);
  ASSERT_EQ(predecessor->endpoint, "192.168.1.1:50199");

  // updating a further node wont affect finger of 128 (199)
  router.update(node{230, "192.168.1.3:50230"});
  ASSERT_EQ(router.predecessor()->uuid, 230);

  predecessor = router.closest_preceding_node(257);
  ASSERT_EQ(predecessor->uuid, 199);
  ASSERT_EQ(predecessor->endpoint, "192.168.1.1:50199");

  predecessor = router.closest_preceding_node(400);
  ASSERT_EQ(predecessor->uuid, 199);
  ASSERT_EQ(predecessor->endpoint, "192.168.1.1:50199");

  router.update(node{600, "192.168.1.1:50600"});
  ASSERT_EQ(router.predecessor()->uuid, 600);

  predecessor = router.closest_preceding_node(1000);
  ASSERT_EQ(predecessor->uuid, 600);
  ASSERT_EQ(predecessor->endpoint, "192.168.1.1:50600");

  predecessor = router.closest_preceding_node(500);
  ASSERT_EQ(predecessor->uuid, 199);
  ASSERT_EQ(predecessor->endpoint, "192.168.1.1:50199");

  ASSERT_EQ(router.predecessor()->uuid, 600);
  std::cout << "\nrouter dump:\n" << router << std::endl;
}

TEST(RouterTest, closest_preceding_node_less_1) {
  Context context = make_context(1);
  Router router(context);

  router.update({100, "127.0.0.1:100"});

  // predecessor of 1 is 100
  uuid_t predecessor = router.closest_preceding_node(1)->uuid;

  ASSERT_EQ(predecessor, 100);
}

TEST(RouterTest, closest_preceding_node_mod) {
  Context context = make_context(999);
  Router router(context);

  // direct successor of 999 is 0
  for (size_t i = 0; i <= 100; i++) {
    router.update({router.calc_successor_uuid_for_index(i), to_string(i)});
  }
  uuid_t predecessor = router.closest_preceding_node(router.calc_successor_uuid_for_index(50))->uuid;

  ASSERT_EQ(predecessor, router.calc_successor_uuid_for_index(49));
}

TEST(RouterTest, closest_preceding_node_mod_2) {
  Context context = make_context(5);
  Router router(context);

  // 5 -> [0]
  router.update({0, "0.0.0.0:50050"});

  const auto predecessor = router.closest_preceding_node(5)->uuid;

  ASSERT_EQ(predecessor, 0);

  ASSERT_TRUE(router.has_predecessor());
  ASSERT_EQ(router.predecessor()->uuid, 0);
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

  auto successor = router.successor();
  ASSERT_FALSE(router.has_predecessor());
  //TODO check
  //ASSERT_FALSE(successor);
}

TEST(RouterTest, closest_preceding_node_mod_3) {
  Context context = make_context(8);
  Router router(context);

  //  4<-8-[1]->4
  router.update({4, "0.0.0.0:50004"});

  const auto predecessor = router.closest_preceding_node(1)->uuid;

  ASSERT_EQ(predecessor, 8);
}

TEST(RouterTest, closest_preceding_node_3) {
  Context context = make_context(8);
  Router router(context);

  // direct successor of 4<-8->10-[1]->4->...
  
  router.update({10, "0.0.0.0:50050"});
  router.update({ 4, "0.0.0.0:50050"});

  const auto predecessor = router.closest_preceding_node(1)->uuid;

  ASSERT_EQ(predecessor, 10);
}

TEST(RouterTest, set_successor_sets_preceding_nodes) {
  Context context;
  RouterSpy router{context};

  //TODO check
  //ASSERT_FALSE(router.successor());

  router.update({5, to_string(5)});

  // --> 0: {1}, 1: {1}, 2: {1}, ..., 9:{1}, 10: {}
  for (size_t i = 0; i < 10; i++) {
    ASSERT_TRUE(router.entry(i).valid());
    ASSERT_EQ(router.entry(i).node().uuid, uuid_t{5});
  }
}

TEST(RouterTest, set_successor_rewrites_same_preceding_nodes) {
  Context context = make_context(0);
  RouterSpy router{context};

  ASSERT_FALSE(router.has_successor());
  //TODO check
  //ASSERT_FALSE(router.successor());

  // [0..5] -> uuid_t{5}
  router.update({32, "32"});
  // [6..10] -> uuid_t{5}
  router.update({128, "128"});

  for (size_t i = 0; i < 7; i++) {
    const auto succ = router.entry(i);
    std::cerr << "i: " << i;
    if(i < 5) {
      ASSERT_EQ(router.entry(i).node().uuid, uuid_t{32});
    } else {
      ASSERT_EQ(router.entry(i).node().uuid, uuid_t{128});
    }
  }

  // insert new node
  router.update({8, "8"});

  for (size_t i = 0; i < 7; i++) {
    const auto succ = router.entry(i);
    if(i < 3) {
      ASSERT_EQ(router.entry(i).node().uuid, uuid_t{8});
    } else if(i < 5) {
      ASSERT_EQ(router.entry(i).node().uuid, uuid_t{32});
    } else {
      ASSERT_EQ(router.entry(i).node().uuid, uuid_t{128});
    }
  }
  const auto node_set = router.get();
  ASSERT_EQ(node_set.size(), 3);
}

TEST(RouterTest, reset_block_last) {
  Context context = make_context(0);
  RouterSpy router{context};

  ASSERT_FALSE(router.has_successor());
  //TODO check
  //ASSERT_FALSE(router.successor());
  ASSERT_FALSE(router.has_predecessor());
  ASSERT_FALSE(router.predecessor());

  // initialize successors in groups(i) of 5
  for (size_t i = 0; i <= 20; i += 5) {
    router.update({router.calc_successor_uuid_for_index(i), "localhost:"+to_string(i)});
  }

  std::cout << "\n\nrouter:\n" << router;

  for (size_t i=5; i < 20; i += 5) {
    for (size_t j=i; j < 5; ++j) {
      ASSERT_EQ("localhost:"+to_string(i), router.entry(i).node().endpoint);
    }
  }

  ASSERT_TRUE(router.has_predecessor());
  ASSERT_TRUE(router.has_successor());

  ASSERT_EQ(router.successor()->uuid, 32);
  ASSERT_EQ(router.predecessor()->uuid, 1048576);

  router.remove({1048576, "localhost:20"});

  std::cout << "\n\nrouter:\n" << router;

  ASSERT_EQ(router.successor()->uuid, 32);
  ASSERT_EQ(router.predecessor()->uuid, 32768);

  for(size_t i=0; i < 32; ++i) {
    if(i<5) 
      ASSERT_EQ(router.entry(i).node().uuid, 32);
    else if(i<10)
      ASSERT_EQ(router.entry(i).node().uuid, 1024);
    else if(i<15)
      ASSERT_EQ(router.entry(i).node().uuid, 32768);
    else
      ASSERT_FALSE(router.entry(i).valid());
  }

  std::cout << "\n\nrouter:\n" << router;
}

TEST(RouterTest, reset_block_middle) {
  Context context = make_context(0);
  RouterSpy router{context};

  ASSERT_FALSE(router.has_successor());
  //TODO: check
  //ASSERT_FALSE(router.successor());
  ASSERT_FALSE(router.has_predecessor());
  ASSERT_FALSE(router.predecessor());

  // initialize successors in groups(i) of 5
  for (size_t i = 0; i < 20; i += 5) {
    router.update({router.calc_successor_uuid_for_index(i), "localhost:"+to_string(i)});
  }

  std::cout << "\n\nrouter:\n" << router;

  ASSERT_EQ(router.successor()->uuid, 32);
  ASSERT_EQ(router.predecessor()->uuid, 32768);

  router.remove({1024, "localhost:15"});

  ASSERT_EQ(router.successor()->uuid, 32);
  ASSERT_EQ(router.predecessor()->uuid, 32768);

  for(size_t i=0; i < 32; ++i) {
    if(i<5) 
      ASSERT_EQ(router.entry(i).node().uuid, 32);
    else if(i<15)
      ASSERT_EQ(router.entry(i).node().uuid, 32768);
    else
      ASSERT_FALSE(router.entry(i).valid());
  }

  std::cout << "\n\nrouter:\n" << router;
}

