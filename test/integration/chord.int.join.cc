#include <string>
#include <thread>

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

    //test::TmpDir& base_dir;

};

TEST_F(JoinTest, bootstrap) {
  const auto base = test::TmpDir(base_dir);
  const auto data0 = base.add_dir("data0");
  const auto meta0 = base.add_dir("meta0");
  const auto port = 50050;
  const auto peer = test::make_peer(test::make_context({"0"}, {bind_addr+std::to_string(port)}, data0, meta0));

  auto t = test::detatch(peer);
  peer->stop();
  t.join();
}

TEST_F(JoinTest, join) {
  const auto base = test::TmpDir(base_dir);

  const auto data0 = base.add_dir("data0");
  const auto meta0 = base.add_dir("meta0");
  const auto port0 = 50050;
  const auto ctxt0 = test::make_context({"0"}, {bind_addr+std::to_string(port0)}, data0, meta0);
  const auto peer0 = test::make_peer(ctxt0);

  const auto data1 = base.add_dir("data1");
  const auto meta1 = base.add_dir("meta1");
  const auto port1 = 50051;
  const auto ctxt1 = test::make_context({"8"}, {bind_addr+std::to_string(port1)}, data1, meta1, ctxt0.advertise_addr, false);
  const auto peer1 = test::make_peer(ctxt1);

  //std::thread t0(&chord::Peer::start, peer0);
  //std::thread t1(&chord::Peer::start, peer1);
  auto t0 = test::detatch(peer0);
  auto t1 = test::detatch(peer1);

  std::this_thread::sleep_for(2s);
  //std::this_thread::sleep_for(15min);

  const auto successor_of_0 = peer0->get_chord()->successor();
  const auto successor_of_1 = peer1->get_chord()->successor();


  std::cerr << "\n\n peer0's successor: " << successor_of_0;
  std::cerr << "\n\n peer1's successor: " << successor_of_1;

  peer0->stop();
  peer1->stop();

  t0.join();
  t1.join();
}
