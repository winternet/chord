#pragma once
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

using chord::test::p;
using chord::test::fn;

using testing::Not;

/**
 * example:
 *   ASSERT_THAT(peer0, Not(ParentContains(uri{"chord:///file0"})));
 */
MATCHER_P(ParentContains, target_uri, "") {
  const auto metadata_mgr = arg->get_metadata_manager();

  const auto filename = target_uri.path().filename();
  const auto parent_path = target_uri.path().parent_path();
  const auto parent_uri = uri{target_uri.scheme(), parent_path};

  const auto parent_exists = metadata_mgr->exists(parent_uri);
  if(!parent_exists) {
    *result_listener << "parent of uri \'" << target_uri << "\' which is \'" << parent_uri << "\' does not exist";
    return false;
  }

  const auto metadata_set = metadata_mgr->get(parent_uri);
  const auto contained = std::any_of(metadata_set.begin(), metadata_set.end(), [&](const fs::Metadata& m) { return m.name == filename; });
  if(!contained) {
    *result_listener << "parent uri \'" << parent_uri << "\' did not contain \'" << filename << "\'";
  }
  return contained;
}

namespace chord {
namespace test {

class IntegrationTest : public ::testing::Test {
  protected:
    const chord::path base_dir{"./integration_test_base_dir"};

    const std::string bind_addr{"127.0.0.1:"};
    TmpDir base{base_dir};

    std::vector<IntPeer*> peers;
    std::vector<std::thread> threads;
    std::shared_ptr<spdlog::logger> logger;

    chord::IntPeer* make_peer(const Context& context) {
      const auto peer = test::make_peer(context);
      peers.push_back(peer);
      threads.push_back(test::start_thread(peer));
      return peer;
    }

    virtual void sleep() const {
      std::this_thread::sleep_for(100ms);
    }

    template<typename Rep, typename Period>
    void sleep(const std::chrono::duration<Rep, Period>& duration) const {
      std::this_thread::sleep_for(duration);
    }

    void TearDown() override {
      logger->debug("[tear down] cleaning up {} peers on {} threads.", peers.size(), threads.size());
      for(auto* p:peers) delete p;
      for(auto& t:threads) t.join();
    }

    void assert_equal(const IntPeer* peer, std::shared_ptr<chord::test::TmpBase> source, const uri& target_uri, const bool check_parent=false) const;
    void assert_deleted(const IntPeer* peer, const uri& target_uri, const bool check_parent=false) const;

    void assert_metadata(const IntPeer* peer, const uri& target_uri) const;
};

std::string put(const path& src, const path& dst);

template<typename TMP>
void put(chord::IntPeer* peer, int replication, const std::shared_ptr<TMP> src, const chord::uri& dst) {
  controller::Client ctrl_client;
  ctrl_client.control(peer->get_context().advertise_addr, "put --repl "+std::to_string(replication)+" "+(src->path.string())+" "+std::string(dst));
}

grpc::Status mkdir(chord::IntPeer* peer, int replication, const chord::uri& dst);

grpc::Status del(chord::IntPeer* peer, const chord::uri& dst, bool recursive=false);

} // namespace test
} // namespace chord

