#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "chord.context.manager.h"

using namespace std;
using namespace chord;

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
      ## uuid
      logging: "1234567890"
      uuid: 1234567890
  )");

  ASSERT_EQ(context.data_directory, "./data-dir");
  ASSERT_EQ(context.meta_directory, "./meta-dir");
  ASSERT_EQ(context.bind_addr, "127.0.0.1:50050");
  ASSERT_EQ(context.join_addr, "127.0.0.1:50051");
  ASSERT_EQ(context.stabilize_period_ms, 5000);
  ASSERT_EQ(context.check_period_ms, 5000);
  ASSERT_EQ(context.uuid(), 1234567890);
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
