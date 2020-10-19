#include <chrono>
#include <future>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <spdlog/spdlog.h>

#include "chord.scheduler.h"

using namespace std;
using namespace chord;
using namespace std::literals;
using std::chrono::system_clock;


TEST(SchedulerTest, schedule_single) {
  Scheduler scheduler;
  spdlog::info("scheduling job for in 200ms");
  const auto now = system_clock::now();
  std::promise<void> pr;

  scheduler.schedule(now + 200ms, [&](){
      spdlog::info("executing job.");
      pr.set_value();
      std::this_thread::sleep_for(5s);
  });

  spdlog::info("waiting for job.");
  ASSERT_EQ(pr.get_future().wait_for(1s), future_status::ready );
  spdlog::info("finished.");
  scheduler.shutdown();
}

TEST(SchedulerTest, schedule_multiple_at_once) {
  Scheduler scheduler;
  const auto now = system_clock::now();
  std::vector<std::promise<void>> jobs;

  for(std::size_t i=0; i < 100; ++i) {
    jobs.emplace_back();
    scheduler.schedule(now + 200ms, [&jobs, i](){
        spdlog::info("executing job {}.", i);
        jobs[i].set_value();
    });
  }

  for(std::size_t i=0; i < 100; ++i) {
    ASSERT_EQ(jobs[i].get_future().wait_for(1s), future_status::ready);
  }
  spdlog::info("finished.");
  scheduler.shutdown();
}

TEST(SchedulerTest, schedule_interleaving) {
  Scheduler scheduler;
  const auto now = system_clock::now();
  std::vector<std::promise<void>> jobs;

  //job0: 0|--------------------------------------------------|400ms
  //job1:                   200ms|--------------|300ms
  //job1:                                           wait|350ms
  //job0:                                                           wait|450ms
  {
    jobs.emplace_back();
    scheduler.schedule(now, [&jobs](){
        spdlog::info("executing job 1.");
        std::this_thread::sleep_for(400ms);
        jobs[0].set_value();
    });
  }
  {
    jobs.emplace_back();
    scheduler.schedule(now + 200ms, [&jobs](){
        spdlog::info("executing job 2.");
        std::this_thread::sleep_for(100ms);
        jobs[1].set_value();
    });
  }

  ASSERT_EQ(jobs[1].get_future().wait_for(350ms), future_status::ready);
  ASSERT_EQ(jobs[0].get_future().wait_for(150ms), future_status::ready);
  spdlog::info("finished.");
  scheduler.shutdown();
}

TEST(SchedulerTest, schedule_duration) {
  Scheduler scheduler;
  const auto now = system_clock::now();

  spdlog::info("schedule_duration.");
  std::vector<std::promise<void>> jobs;
  //job0:                                      V (cnt=0)                                V (cnt=1)
  //job0: 0|-----------------------------------|200ms-----------------------------------|400ms-----------------------------------|600ms
  //job1:                                                         350ms|---------------------------------|450ms
  std::atomic<int> cnt = 0;
  jobs.emplace_back();
  scheduler.schedule(chrono::milliseconds(200), [&jobs, &cnt]() mutable {
      //spdlog::info("executing job 1.");
      std::this_thread::sleep_for(200ms);
      //spdlog::info("executed job 1 - signalling.");
      if(cnt++ == 0) jobs[0].set_value();
      //spdlog::info("executed job 1 - exiting.");
  });

  jobs.emplace_back();
  scheduler.schedule(now + 350ms, [&jobs](){
      //spdlog::info("executing job 2.");
      std::this_thread::sleep_for(100ms);
      //spdlog::info("executed job 2 - signalling.");
      jobs[1].set_value();
      //spdlog::info("executed job 2 - exiting.");
  });

  spdlog::info("waiting for job 1.");
  //ASSERT_EQ(jobs[0].get_future().wait_for(440ms), future_status::ready);
  ASSERT_EQ(jobs[0].get_future().wait_for(5min), future_status::ready);
  spdlog::info("waiting for job 2.");
  ASSERT_EQ(jobs[1].get_future().wait_for(100ms), future_status::ready);
  spdlog::info("finished.");
  scheduler.shutdown();
}
