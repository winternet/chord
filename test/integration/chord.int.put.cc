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

public:
    void assert_equal(const IntPeer* peer, std::shared_ptr<chord::test::TmpBase> source, const uri& target_uri) const {
      const auto metadata_mgr = peer->get_metadata_manager();
      const auto target_path = peer->get_context().data_directory/target_uri.path();

      ASSERT_TRUE(file::files_equal(p(source), target_path)) << "source[" << source->path << "] vs [" << target_path << "]";
      ASSERT_TRUE(metadata_mgr->exists(target_uri));
    }

};

std::string put(const path& src, const path& dst) {
  return "put " + src.string() + " " + to_string(uri{"chord", dst});
}

template<typename TMP>
void put(chord::IntPeer* peer, int replication, const std::shared_ptr<TMP> src, const chord::uri& dst) {
  controller::Client ctrl_client;
  ctrl_client.control(peer->get_context().advertise_addr, "put --repl "+std::to_string(replication)+" "+(src->path.string())+" "+std::string(dst));
}

TEST_F(PutTest, nodes_1__repl_1__file_1) {
  const auto source0 = base.add_dir("source0");
  const auto file0 = source0->add_file("file0");

  const auto data0 = base.add_dir("data0");
  const auto meta0 = base.add_dir("meta0");
  const auto port = 50050;
  const auto peer = make_peer(test::make_context({"0"}, {bind_addr+std::to_string(port)}, data0, meta0));
  const auto root_uri = uri{"chord:///"};

  ASSERT_TRUE(file::is_empty(data0->path));
  put(peer, 1, file0, root_uri);

  assert_equal(peer, file0, uri{"chord:///file0"});
}

TEST_F(PutTest, nodes_1__repl_1__folder) {
  const auto source0 = base.add_dir("source0");
  const auto file0 = source0->add_file("file0.md");
  const auto file1 = source0->add_file("file1.md");

  const auto data0 = base.add_dir("data0");
  const auto meta0 = base.add_dir("meta0");
  const auto port = 50050;
  const auto peer = make_peer(test::make_context({"0"}, {bind_addr+std::to_string(port)}, data0, meta0));
  const auto metadata_mgr = peer->get_metadata_manager();
  const auto root_uri = uri{"chord:///"};

  ASSERT_TRUE(file::is_empty(data0->path));
  put(peer, 1, source0, root_uri);

  ASSERT_TRUE(metadata_mgr->exists(root_uri));
  ASSERT_TRUE(metadata_mgr->exists(uri{"chord:///source0"}));

  assert_equal(peer, file0, uri{"chord:///source0/file0.md"});
  assert_equal(peer, file1, uri{"chord:///source0/file1.md"});
}

/**
 * PUT [replication: 1]
 * source0
 * ├── file0
 * └── file1
 * 
 * EXPECTED
 * data0
 * └── file0
 * data1
 * └── file1
 */
TEST_F(PutTest,  nodes_2__repl_1__folder__height_1) {
  const auto root_uri = uri{"chord:///"};
  const auto source0 = base.add_dir("source0");
  const auto file0 = source0->add_file("file0.md");
  const auto file1 = source0->add_file("file1.md");

  const auto data0 = base.add_dir("data0");
  const auto ctxt0 = test::make_context({"0"}, 
      {bind_addr+"50050"}, data0, base.add_dir("meta0"));
  const auto peer0 = make_peer(ctxt0);
  ASSERT_TRUE(file::is_empty(data0->path));

  const auto data1 = base.add_dir("data1");
  const auto ctxt1 = test::make_context({"28948022309329048855892746252171976963317496166410141009864396001978282409984"}, 
      {bind_addr+"50051"}, data1, base.add_dir("meta1"), ctxt0.advertise_addr, false);
  const auto peer1 = make_peer(ctxt1);
  ASSERT_TRUE(file::is_empty(data1->path));

  put(peer0, 1, source0, root_uri);

  // file0 -> peer0 
  // file1 -> peer1
  assert_equal(peer0, file0, uri{"chord:///source0/file0.md"});
  assert_equal(peer1, file1, uri{"chord:///source0/file1.md"});
  peer0->get_metadata_manager()->exists(uri{"chord:///source0"});
}

/**
 * PUT [replication: 2]
 * source0
 * ├── file0
 * └── file1
 * 
 * EXPECTED
 * data0
 * ├── file0
 * └── file1
 * data1
 * ├── file0
 * └── file1
 */
TEST_F(PutTest, nodes_2__repl_2__folder__height_1) {
  const auto root_uri = uri{"chord:///"};
  const auto source0 = base.add_dir("source0");
  const auto file0 = source0->add_file("file0.md");
  const auto file1 = source0->add_file("file1.md");

  const auto data0 = base.add_dir("data0");
  const auto ctxt0 = test::make_context({"0"}, 
      {bind_addr+"50050"}, data0, base.add_dir("meta0"));
  const auto peer0 = make_peer(ctxt0);
  ASSERT_TRUE(file::is_empty(data0->path));

  const auto data1 = base.add_dir("data1");
  const auto ctxt1 = test::make_context({"28948022309329048855892746252171976963317496166410141009864396001978282409984"}, 
      {bind_addr+"50051"}, data1, base.add_dir("meta1"), ctxt0.advertise_addr, false);
  const auto peer1 [[maybe_unused]] = make_peer(ctxt1);
  ASSERT_TRUE(file::is_empty(data1->path));

  put(peer0, 2, source0, root_uri);

  ASSERT_EQ(peers.size(), 2);
  for(const auto peer : peers) {
    assert_equal(peer, file0, uri{"chord:///source0/file0.md"});
    assert_equal(peer, file1, uri{"chord:///source0/file1.md"});
    peer->get_metadata_manager()->exists(uri{"chord:///source0"});
    peer->get_metadata_manager()->exists(uri{"chord:///"});
  }
}

/**
 * PUT [replication: 1]
 * source0/
 * ├── file0.md
 * ├── file1.md
 * └── subdir
 *     ├── subfile0.md
 *     ├── subfile1.md
 *     └── subsubdir
 *         └── subsubfile0.md
 * 
 * EXPECTED
 * data0
 * └── source0
 *     ├── file0.md
 *     └── subdir
 *         └── subfile0.md
 * data1
 * └── source0
 *     ├── file1.md
 *     └── subdir
 *         ├── subfile1.md
 *         └── subsubdir
 *             └── subsubfile0.md
 * 
 */
TEST_F(PutTest, nodes_2__repl_1__folder__height_3) {
  const auto root_uri = uri{"chord:///"};
  const auto source0 = base.add_dir("source0");
  const auto file0 = source0->add_file("file0.md");
  const auto file1 = source0->add_file("file1.md");

  const auto subdir = source0->add_dir("subdir");
  const auto subfile0 = subdir->add_file("subfile0.md");
  const auto subfile1 = subdir->add_file("subfile1.md");

  const auto subsubdir = subdir->add_dir("subsubdir");
  const auto subsubfile0 = subsubdir->add_file("subsubfile0.md");

  const auto data0 = base.add_dir("data0");
  const auto ctxt0 = test::make_context({"0"}, 
      {bind_addr+"50050"}, data0, base.add_dir("meta0"));
  const auto peer0 = make_peer(ctxt0);
  ASSERT_TRUE(file::is_empty(data0->path));

  const auto data1 = base.add_dir("data1");
  const auto ctxt1 = test::make_context({"28948022309329048855892746252171976963317496166410141009864396001978282409984"}, 
      {bind_addr+"50051"}, data1, base.add_dir("meta1"), ctxt0.advertise_addr, false);
  const auto peer1 = make_peer(ctxt1);
  ASSERT_TRUE(file::is_empty(data1->path));

  put(peer0, 1, source0, root_uri);

  {
    const auto metadata_mgr = peer0->get_metadata_manager();
    assert_equal(peer0, file0,       uri{"chord:///source0/file0.md"});
    assert_equal(peer0, subfile0,    uri{"chord:///source0/subdir/subfile0.md"});
    assert_equal(peer0, subsubfile0, uri{"chord:///source0/subdir/subsubdir/subsubfile0.md"});
    ASSERT_TRUE(metadata_mgr->exists(uri{"chord:///source0"}));
    ASSERT_TRUE(metadata_mgr->exists(uri{"chord:///source0/subdir/subsubdir"}));
  }
  {
    const auto metadata_mgr = peer1->get_metadata_manager();
    assert_equal(peer1, file1,    uri{"chord:///source0/file1.md"});
    assert_equal(peer1, subfile1, uri{"chord:///source0/subdir/subfile1.md"});
    ASSERT_TRUE(metadata_mgr->exists(uri{"chord:///source0/subdir"}));
  }
}

/**
 * PUT [replication: 2]
 * source0/
 * ├── file0.md
 * ├── file1.md
 * └── subdir
 *     ├── subfile0.md
 *     ├── subfile1.md
 *     └── subsubdir
 *         └── subsubfile0.md
 * 
 * EXPECTED
 * data0
 * ├── file0.md
 * ├── file1.md
 * └── subdir
 *     ├── subfile0.md
 *     ├── subfile1.md
 *     └── subsubdir
 *         └── subsubfile0.md
 * data1
 * ├── file0.md
 * ├── file1.md
 * └── subdir
 *     ├── subfile0.md
 *     ├── subfile1.md
 *     └── subsubdir
 *         └── subsubfile0.md
 */
TEST_F(PutTest, nodes_3__repl_3__folder__height_3) {
  const auto root_uri = uri{"chord:///"};
  const auto source0 = base.add_dir("source0");
  const auto file0 = source0->add_file("file0.md");
  const auto file1 = source0->add_file("file1.md");

  const auto subdir = source0->add_dir("subdir");
  const auto subfile0 = subdir->add_file("subfile0.md");
  const auto subfile1 = subdir->add_file("subfile1.md");

  const auto subsubdir = subdir->add_dir("subsubdir");
  const auto subsubfile0 = subsubdir->add_file("subsubfile0.md");

  const auto data0 = base.add_dir("data0");
  const auto ctxt0 = test::make_context({"0"}, 
      {bind_addr+"50050"}, data0, base.add_dir("meta0"));
  const auto peer0 [[maybe_unused]] = make_peer(ctxt0);
  ASSERT_TRUE(file::is_empty(data0->path));

  const auto data1 = base.add_dir("data1");
  const auto ctxt1 = test::make_context({"8"}, 
      {bind_addr+"50051"}, data1, base.add_dir("meta1"), ctxt0.advertise_addr, false);
  const auto peer1 [[maybe_unused]] = make_peer(ctxt1);
  ASSERT_TRUE(file::is_empty(data1->path));

  const auto data2 = base.add_dir("data2");
  const auto ctxt2 = test::make_context({"512"}, 
      {bind_addr+"50052"}, data2, base.add_dir("meta2"), ctxt0.advertise_addr, false);
  const auto peer2 [[maybe_unused]] = make_peer(ctxt2);
  ASSERT_TRUE(file::is_empty(data2->path));

  put(peer0, 3, source0, root_uri);

  ASSERT_EQ(peers.size(), 3);
  for(const auto peer:peers)
  {
    const auto metadata_mgr = peer->get_metadata_manager();
    assert_equal(peer, file0,       uri{"chord:///source0/file0.md"});
    assert_equal(peer, file1,       uri{"chord:///source0/file1.md"});
    assert_equal(peer, subfile0,    uri{"chord:///source0/subdir/subfile0.md"});
    assert_equal(peer, subfile1,    uri{"chord:///source0/subdir/subfile1.md"});
    assert_equal(peer, subsubfile0, uri{"chord:///source0/subdir/subsubdir/subsubfile0.md"});
    ASSERT_TRUE(metadata_mgr->exists(uri{"chord:///source0"}));
    ASSERT_TRUE(metadata_mgr->exists(uri{"chord:///source0/subdir"}));
    ASSERT_TRUE(metadata_mgr->exists(uri{"chord:///source0/subdir/subsubdir"}));
  }
}
