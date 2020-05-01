#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <iterator>
#include <condition_variable>
#include <mutex>

#include "chord.context.h"
#include "chord.fs.monitor.h"
#include "util/chord.test.tmp.file.h"
#include "util/chord.test.tmp.dir.h"

//TODO remove
#include <libfswatch/c++/monitor.hpp>

using chord::test::TmpFile;
using chord::test::TmpDir;

using ::testing::Contains;
using ::testing::IsEmpty;

//class MonitorTest : public ::testing::Test {
//  protected:
//    void SetUp() override {
//    }
//};

void sleep() {
  using namespace std::chrono_literals;
  std::this_thread::sleep_for(350ms);
}

chord::Context make_context(const chord::path& data_dir) {
  chord::Context ctxt;
  ctxt.data_directory = data_dir;
  return ctxt;
}

TEST(monitor, create_file) {
  const TmpDir tmpDir;
  chord::fs::monitor mon(make_context(tmpDir.path));
  bool callback_invoked = false;

  std::mutex mtx;
  std::condition_variable cv;

  mon.events().connect([&](const std::vector<chord::fs::monitor::event> events) {
      std::copy(events.begin(), events.end(), std::ostream_iterator<chord::fs::monitor::event>(std::cout, "\n"));

      ASSERT_GE(events.size(), 1);
      const auto ev = events.at(0);
      ASSERT_THAT(ev.flags, Contains(chord::fs::monitor::event::flag::CREATED));
      std::lock_guard<std::mutex> lck(mtx);
      callback_invoked = true;
      cv.notify_one();
      mon.stop();
  });
  tmpDir.add_file("foo");

  std::unique_lock<std::mutex> lock(mtx);
  cv.wait(lock, [&]{return callback_invoked;});
  
  ASSERT_TRUE(callback_invoked);
}

TEST(monitor, remove_file) {
  const TmpDir tmpDir;
  const auto file = tmpDir.add_file("foo");

  chord::fs::monitor mon(make_context(tmpDir.path));

  std::mutex mtx;
  std::condition_variable cv;
  bool callback_invoked = false;

  mon.events().connect([&](const std::vector<chord::fs::monitor::event> events) {
      std::copy(events.begin(), events.end(), std::ostream_iterator<chord::fs::monitor::event>(std::cout, "\n"));

      ASSERT_GE(events.size(), 1);
      const auto ev = events.at(0);
      ASSERT_THAT(ev.flags, Contains(chord::fs::monitor::event::flag::REMOVED));
      std::lock_guard<std::mutex> lck(mtx);
      callback_invoked = true;
      cv.notify_one();
      mon.stop();
  });
  file.remove();

  std::unique_lock<std::mutex> lock(mtx);
  cv.wait(lock, [&]{return callback_invoked;});
  
  ASSERT_TRUE(callback_invoked);
}

TEST(monitor, filter_file) {
  const TmpDir tmpDir;
  chord::fs::monitor mon(make_context(tmpDir.path));

  bool callback_invoked = false;

  mon.events().connect([&]([[maybe_unused]] const std::vector<chord::fs::monitor::event> events) {
      std::copy(events.begin(), events.end(), std::ostream_iterator<chord::fs::monitor::event>(std::cout, "\n"));
      callback_invoked = true;
  });
  mon.add_filter({tmpDir.path / "foo", 3}); // create, update, remove
  const auto file = tmpDir.add_file("foo");
  mon.stop();

  sleep();

  //ASSERT_THAT(mon.filters(), IsEmpty());
  ASSERT_FALSE(callback_invoked);
}

TEST(monitor, filter_file_by_flag) {
  const TmpDir tmpDir;
  chord::fs::monitor mon(make_context(tmpDir.path));
  bool callback_invoked = false;
  mon.events().connect([&]([[maybe_unused]] const std::vector<chord::fs::monitor::event> events) {
      std::copy(events.begin(), events.end(), std::ostream_iterator<chord::fs::monitor::event>(std::cout, "\n"));
      callback_invoked = true;
  });
  mon.add_filter({tmpDir.path / "foo", 1, chord::fs::monitor::event::flag::CREATED});
  mon.add_filter({tmpDir.path / "foo", 2, chord::fs::monitor::event::flag::UPDATED});
  const auto file = tmpDir.add_file("foo");
  mon.stop();

  sleep();
  
  //ASSERT_THAT(mon.filters(), IsEmpty());
  ASSERT_FALSE(callback_invoked);
}
