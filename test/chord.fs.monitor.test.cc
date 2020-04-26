#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <iterator>

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
  std::this_thread::sleep_for(300ms);
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
  mon.events().connect([&](const std::vector<chord::fs::monitor::event> events) {
      std::copy(events.begin(), events.end(), std::ostream_iterator<chord::fs::monitor::event>(std::cout, "\n"));

      ASSERT_GE(events.size(), 1);
      const auto ev = events.at(0);
      ASSERT_THAT(ev.flags, Contains(chord::fs::monitor::event::flag::CREATED));
      callback_invoked = true;
      mon.stop();
  });
  tmpDir.add_file("foo");

  sleep();
  
  ASSERT_TRUE(callback_invoked);
}

TEST(monitor, remove_file) {
  const TmpDir tmpDir;
  const auto file = tmpDir.add_file("foo");

  chord::fs::monitor mon(make_context(tmpDir.path));
  bool callback_invoked = false;
  mon.events().connect([&](const std::vector<chord::fs::monitor::event> events) {
      std::copy(events.begin(), events.end(), std::ostream_iterator<chord::fs::monitor::event>(std::cout, "\n"));

      ASSERT_GE(events.size(), 1);
      const auto ev = events.at(0);
      ASSERT_THAT(ev.flags, Contains(chord::fs::monitor::event::flag::REMOVED));
      callback_invoked = true;
      mon.stop();
  });
  file.remove();


  sleep();
  
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

  sleep();

  ASSERT_THAT(mon.filters(), IsEmpty());
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
  const auto file = tmpDir.add_file("foo");
  mon.stop();

  sleep();
  
  ASSERT_THAT(mon.filters(), IsEmpty());
  ASSERT_FALSE(callback_invoked);
}
