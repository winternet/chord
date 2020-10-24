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
};

} // namespace test
} // namespace chord

