#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <iterator>
#include <condition_variable>
#include <mutex>
#include <atomic>

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

using namespace std::literals::chrono_literals;


static constexpr auto DEFAULT_DURATION = 5s;

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

  std::mutex mtx;
  std::condition_variable cv;

  bool callback_invoked = false;

  mon.events().connect([&](const std::vector<chord::fs::monitor::event> events) {
      std::copy(events.begin(), events.end(), std::ostream_iterator<chord::fs::monitor::event>(std::cout, "\n"));

      ASSERT_EQ(events.size(), 1);
      ASSERT_THAT(events.at(0).flags, Contains(chord::fs::monitor::event::flag::CREATED));

      std::unique_lock<std::mutex> lck(mtx);
      callback_invoked = true;
      cv.notify_one();
  });

  const auto holder = tmpDir.add_file("foo");

  std::unique_lock<std::mutex> lck(mtx);
  cv.wait_for(lck, DEFAULT_DURATION, [&]{return callback_invoked;});
  
  ASSERT_TRUE(callback_invoked);
}

TEST(monitor, create_directories) {
  const TmpDir tmpDir;
  chord::fs::monitor mon(make_context(tmpDir.path));
  bool callback_invoked = false;

  std::mutex mtx;
  std::condition_variable cv;

  mon.events().connect([&](const std::vector<chord::fs::monitor::event> events) {
      std::copy(events.begin(), events.end(), std::ostream_iterator<chord::fs::monitor::event>(std::cout, "\n"));
      std::unique_lock<std::mutex> lck(mtx);
      callback_invoked = true;
      cv.notify_one();
  });

  const auto sub_dir = tmpDir.add_dir();
  const auto sub_sub_dir = sub_dir->add_dir();

  std::unique_lock<std::mutex> lck(mtx);
  cv.wait_for(lck, DEFAULT_DURATION, [&]{return callback_invoked;});

  mon.stop();

  ASSERT_FALSE(callback_invoked);
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

      ASSERT_EQ(events.size(), 1);
      ASSERT_THAT(events.at(0).flags, Contains(chord::fs::monitor::event::flag::REMOVED));
      std::unique_lock<std::mutex> lck(mtx);
      callback_invoked = true;
      cv.notify_one();
  });

  file->remove();

  std::unique_lock<std::mutex> lck(mtx);
  cv.wait_for(lck, DEFAULT_DURATION, [&]{return callback_invoked;});
  
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

  mon.add_filter({tmpDir.path / "foo"}); // create, update, remove
  const auto file = tmpDir.add_file("foo");

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

  mon.add_filter({tmpDir.path / "foo", chord::fs::monitor::event::flag::UPDATED});
  const auto file = tmpDir.add_file("foo");

  ASSERT_FALSE(callback_invoked);
}

TEST(monitor, directories) {
  const TmpDir base;

  chord::fs::monitor mon(make_context(base.path));

  const auto source0 = base.add_dir("source0");
  const auto file0 = source0->add_file("file0.md");
  const auto file1 = source0->add_file("file1.md");

  const auto subdir = source0->add_dir("subdir");
  const auto subfile0 = subdir->add_file("subfile0.md");
  const auto subfile1 = subdir->add_file("subfile1.md");

  const auto subsubdir = subdir->add_dir("subsubdir");
  const auto subsubfile0 = subsubdir->add_file("subsubfile0.md");


  std::mutex mtx;
  std::unique_lock<std::mutex> lck(mtx);
  std::condition_variable cv;

  std::atomic<size_t> counter = 5;

  mon.events().connect([&]([[maybe_unused]] const std::vector<chord::fs::monitor::event> events) {
      std::copy(events.begin(), events.end(), std::ostream_iterator<chord::fs::monitor::event>(std::cout, "\n"));
      counter -= events.size();
      if(counter == 0) cv.notify_one();
  });

  cv.wait_for(lck, DEFAULT_DURATION, [&]{return counter == 0;});

  ASSERT_EQ(counter, 0);

  mon.stop();
}
