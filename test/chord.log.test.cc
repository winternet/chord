#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "chord.log.h"
#include "chord.exception.h"

using namespace std;

using chord::log::Formatter;
using chord::log::Sink;
using chord::log::SinkType;

TEST(chord_log_formatter, equals) {
  Formatter formatter0{"aaaaa"};
  Formatter formatter1{"aaaaa"};
  ASSERT_EQ(formatter0, formatter1);
}

TEST(chord_log_formatter, operator_less) {
  Formatter formatter0{"aaaaa"};
  Formatter formatter1{"bbbbb"};
  ASSERT_LT(formatter0, formatter1);
  ASSERT_FALSE(formatter1 < formatter0);
}

TEST(chord_log_sink_type, from_string) {
  ASSERT_EQ(SinkType::CONSOLE, SinkType::from("console"));
  ASSERT_NE(SinkType::from("file"), SinkType::from("console"));
  ASSERT_THROW(SinkType::from("-_-unknown-_-"), chord::exception);
}

TEST(chord_log_sink, default_sink) {
  Sink default_console;
  ASSERT_EQ(SinkType::CONSOLE, default_console.type);
}

