#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "chord.context.manager.h"

using namespace std;
using namespace chord;

TEST(chord_context_manager, parse_valid_config) {
  Context context = ContextManager::load(
      "\nversion: 1"
      "\n## folders"
      "\ndata-directory: \"./data-dir\""
      "\nmeta-directory: \"./meta-dir\""
      "\n## networking"
      "\nbind-addr: \"127.0.0.1:50050\""
      "\njoin-addr: \"127.0.0.1:50050\""
      "\n## details"
      "\nstabilize-ms: 5000"
      "\ncheck-ms: 5000"
      "\n## uuid"
      "\nuuid: \"1234567890\""
  );

  ASSERT_EQ(context.data_directory, "./data-dir");
  ASSERT_EQ(context.meta_directory, "./meta-dir");
  ASSERT_EQ(context.bind_addr, "127.0.0.1:50050");
  ASSERT_EQ(context.join_addr, "127.0.0.1:50050");
  ASSERT_EQ(context.stabilize_period_ms, 5000);
  ASSERT_EQ(context.check_period_ms, 5000);
  ASSERT_EQ(context.uuid(), 1234567890);
}

