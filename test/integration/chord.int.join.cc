#include <string>
#include <thread>
#include <memory>
#include <vector>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "../util/chord.test.tmp.dir.h"
#include "../util/chord.test.helper.h"

using namespace std;

using namespace chord;

class JoinTest : public ::testing::Test {
  protected:
    const chord::path base_dir{"./integration_test_base_dir"};
    const std::string bind_addr{"127.0.0.1:"};
    chord::test::TmpDir base{base_dir};

    std::vector<chord::Peer*> peers;
    std::vector<std::thread> threads;
    std::shared_ptr<spdlog::logger> logger;

    chord::IntPeer* make_peer(const Context& context) {
      const auto peer = test::make_peer(context);
      peers.push_back(peer);
      threads.push_back(test::detatch(peer));
      return peer;
    }

    void SetUp() {
      logger = chord::log::get_or_create("int.test.join");
    }

    void TearDown() {
      logger->debug("[tear down] cleaning up {} peers on {} threads.", peers.size(), threads.size());
      for(auto* p:peers) delete p;
      for(auto& t:threads) t.join();
    }
};

TEST_F(JoinTest, bootstrap) {
  const auto data0 = base.add_dir("data0");
  const auto meta0 = base.add_dir("meta0");
  const auto port = 50050;
  const auto peer = make_peer(test::make_context({"0"}, {bind_addr+std::to_string(port)}, data0, meta0));
  std::this_thread::sleep_for(1s);
  peer->stop();
}

TEST_F(JoinTest, join) {
  const auto data0 = base.add_dir("data0");
  const auto meta0 = base.add_dir("meta0");
  const auto port0 = 50050;
  const auto ctxt0 = test::make_context({"0"}, {bind_addr+std::to_string(port0)}, data0, meta0);
  const auto peer0 = make_peer(ctxt0);

  const auto data1 = base.add_dir("data1");
  const auto meta1 = base.add_dir("meta1");
  const auto port1 = 50051;
  const auto ctxt1 = test::make_context({"8"}, {bind_addr+std::to_string(port1)}, data1, meta1, ctxt0.advertise_addr, false);
  const auto peer1 = make_peer(ctxt1);

  std::this_thread::sleep_for(1s);

  const auto successor_of_0 = peer0->get_chord()->successor();
  const auto successor_of_1 = peer1->get_chord()->successor();

  logger->info("peer0's ({}) successor {}", ctxt0.node(), successor_of_0);
  logger->info("peer1's ({}) successor {}", ctxt1.node(), successor_of_0);

  ASSERT_EQ(successor_of_0, ctxt1.node());
  ASSERT_EQ(successor_of_1, ctxt0.node());
}