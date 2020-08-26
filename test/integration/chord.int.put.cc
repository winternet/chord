#include <string>
#include <thread>
#include <memory>
#include <vector>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "chord.int.test.h"
#include "../util/chord.test.tmp.dir.h"
#include "../util/chord.test.helper.h"

using namespace std;
using namespace chord;

using chord::test::p;
using chord::test::fn;

class PutTest : public chord::test::IntegrationTest {

    void SetUp() override {
      logger = chord::log::get_or_create("int.test.put");
    }

};

std::string put(const path& src, const path& dst) {
  return "put " + src.string() + " " + to_string(uri{"chord", dst});
}

template<typename S>
void put(chord::IntPeer* peer, const S src, const chord::uri& dst) {
  controller::Client ctrl_client;
  ctrl_client.control(peer->get_context().advertise_addr, 
      "put "+(src->path.string())+" "+std::string(dst));
}

TEST_F(PutTest, single_node_put_single_file) {
  const auto source0 = base.add_dir("source0");
  const auto file0 = source0->add_file("file0");

  const auto data0 = base.add_dir("data0");
  const auto meta0 = base.add_dir("meta0");
  const auto port = 50050;
  const auto peer = make_peer(test::make_context({"0"}, {bind_addr+std::to_string(port)}, data0, meta0));

  ASSERT_TRUE(file::is_empty(data0->path));
  put(peer, file0, uri{"chord:///"});
  ASSERT_TRUE(file::files_equal(p(file0), p(data0)/fn(file0)));
}

TEST_F(PutTest, single_node_put_folder) {
  const auto source0 = base.add_dir("source0");
  const auto file0 = source0->add_file("file0");
  const auto file1 = source0->add_file("file1");

  const auto data0 = base.add_dir("data0");
  const auto meta0 = base.add_dir("meta0");
  const auto port = 50050;
  const auto peer = make_peer(test::make_context({"0"}, {bind_addr+std::to_string(port)}, data0, meta0));

  ASSERT_TRUE(file::is_empty(data0->path));
  put(peer, source0, uri{"chord:///"});
  ASSERT_TRUE(file::files_equal(p(file0), p(data0)/fn(source0)/fn(file0)));
  ASSERT_TRUE(file::files_equal(p(file1), p(data0)/fn(source0)/fn(file1)));
}

//  std::this_thread::sleep_for(1s);
