#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "chord.context.manager.h"

using namespace std;
using namespace chord;
using namespace chord::log;

TEST(chord_context_manager, parse_valid_config) {
  Context context = ContextManager::load(R"(
      version: 1
      ## folders
      data-directory: "./data-dir"
      meta-directory: "./meta-dir"
      ## networking
      bind-addr: 127.0.0.1:50050
      join-addr: 127.0.0.1:50051
      ## details
      stabilize-ms: 5000
      check-ms: 5000
      logging:
        level: debug
        formatters:
          CUSTOMFORMAT:
            pattern: "*** [%H:%M:%S %z] [thread %t] %v ***"
        sinks:
          DEFAULT:
            type: "file"
            path: "/tmp/foo"
            formatter: CUSTOMFORMAT
            level: debug
          CONS:
            type: console
        loggers:
          MYLOG:
            sinks: [DEFAULT, CONS]
          OTHER:
            filter: ^chord[.](?!fs)
            level: trace
      ## uuid
      uuid: 1234567890
  )");

  ASSERT_EQ(context.data_directory, "./data-dir");
  ASSERT_EQ(context.meta_directory, "./meta-dir");
  ASSERT_EQ(context.bind_addr, "127.0.0.1:50050");
  ASSERT_EQ(context.join_addr, "127.0.0.1:50051");
  ASSERT_EQ(context.stabilize_period_ms, 5000);
  ASSERT_EQ(context.check_period_ms, 5000);
  ASSERT_EQ(context.uuid(), 1234567890);
  auto formatters = context.logging.formatters;
  ASSERT_EQ(formatters.size(), 1);
  ASSERT_EQ(formatters["CUSTOMFORMAT"].pattern, "*** [%H:%M:%S %z] [thread %t] %v ***");

  ASSERT_EQ(context.logging.level, "debug");

  auto sinks = context.logging.sinks;
  ASSERT_EQ(sinks.size(), 2);
  ASSERT_EQ(sinks["DEFAULT"].type, SinkType::FILE);
  ASSERT_EQ(sinks["DEFAULT"].path, std::string("/tmp/foo"));
  ASSERT_EQ(sinks["DEFAULT"].level, std::string("debug"));
  ASSERT_EQ(sinks["DEFAULT"].formatter->pattern, formatters["CUSTOMFORMAT"].pattern);
  ASSERT_EQ(sinks["CONS"].type, SinkType::CONSOLE);

  auto loggers = context.logging.loggers;
  ASSERT_EQ(loggers.size(), 2);
  ASSERT_EQ(loggers["MYLOG"].sinks.size(), 2);
  ASSERT_EQ(loggers["OTHER"].filter, "^chord[.](?!fs)");
  ASSERT_EQ(loggers["OTHER"].level, "trace");
}

TEST(chord_context_manager, parse_invalid_config__equal_bind_join_addresses) {
  ASSERT_THROW(ContextManager::load(R"(
      version: 1
      ## folders
      data-directory: "./data-dir"
      meta-directory: ./meta-dir
      ## networking
      bind-addr: "127.0.0.1:50050"
      join-addr: "127.0.0.1:50050"
      ## details
      stabilize-ms: 5000
      check-ms: 5000
      ## uuid
      uuid: "1234567890")"
  ), chord::exception);
}

TEST(chord_context_manager, parse_invalid_config__equal_data_meta_directories) {
  ASSERT_THROW(ContextManager::load(R"(
      version: 1
      ## folders
      data-directory: "./same-dir"
      meta-directory: "./same-dir"
      ## networking
      bind-addr: "127.0.0.1:50050"
      join-addr: "127.0.0.1:50051"
      ## details
      stabilize-ms: 5000
      check-ms: 5000
      ## uuid
      uuid: "1234567890")"
  ), chord::exception);
}
