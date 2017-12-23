#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <bitset>

#include "chord.crypto.h"

using namespace std;
using namespace chord;

TEST(chord_uuid, value_constructor) {
  auto str = string{"1234"};
  auto number = int{1234};

  auto uuid_str = uuid_t{str};
  auto uuid_ptr = uuid_t{number};

  ASSERT_EQ(uuid_str, uuid_ptr);
}

TEST(chord_uuid, iterator_constructor) {
  //10101010|10101010|10101010|10101010 = 1431655765
  auto arr = array<int, 4>{1431655765, 1431655765, 1431655765, 1431655765};  // 4xsizeof(int) = 16 byte
  auto uuid_str = uuid_t{"113427455640312821154458202477256070485"};
  auto uuid_ptr = uuid_t{arr.begin(), arr.end()};

  ASSERT_EQ(uuid_str, uuid_ptr);
}

TEST(chord_uuid, raw_pointer_constructor) {
  auto bits = bitset<256>{}; //256 bit / 8 bit / 4 = 8 byte
  for (int i = 0; i < 256; i++)
    if (i%2==0)
      bits.set(i, true);
    else
      bits.set(i, false);

  auto uuid_str = uuid_t{"38597363079105398474523661669562635951089994888546854679819194669304376546645"};
  auto uuid_ptr = uuid_t{&bits, &bits + 1};

  ASSERT_EQ(uuid_str, uuid_ptr);
}

TEST(chord_uuid, random) {
  for (int i = 0; i < 100; i++) {
    auto a = uuid::random();
    auto b = uuid::random();
    ASSERT_NE(a, b);
  }
}

TEST(chord_uuid, operator_plus_equals) {
  auto id = uuid_t{5};
  ASSERT_EQ(id, 5);
  ASSERT_EQ(id += 5, 10);
  ASSERT_EQ(id, 10);
}

TEST(chord_uuid, operator_minus_equals) {
  auto id = uuid_t{5};
  ASSERT_EQ(id, 5);
  ASSERT_EQ(id -= 5, 0);
  ASSERT_EQ(id, 0);
}

TEST(chord_uuid, operator_mul_equals) {
  auto id = uuid_t{5};
  ASSERT_EQ(id, 5);
  ASSERT_EQ(id *= 2, 10);
  ASSERT_EQ(id, 10);
}

TEST(chord_uuid, operator_div_equals) {
  auto id = uuid_t{15};
  ASSERT_EQ(id, 15);
  ASSERT_EQ(id /= 3, 5);
  ASSERT_EQ(id, 5);
}

TEST(chord_uuid, operator_minus) {
  auto id = uuid_t{15};
  ASSERT_EQ(id, 15);
  ASSERT_EQ(id - 5, 10);
  ASSERT_EQ(id, 15);
}

TEST(chord_uuid, operator_plus) {
  auto id = uuid_t{15};
  ASSERT_EQ(id, 15);
  ASSERT_EQ(id + 5, 20);
  ASSERT_EQ(id, 15);
}

TEST(chord_uuid, operator_mul) {
  auto id = uuid_t{5};
  ASSERT_EQ(id, 5);
  ASSERT_EQ(id*3, 15);
  ASSERT_EQ(id, 5);
}

TEST(chord_uuid, operator_div) {
  auto id = uuid_t{15};
  ASSERT_EQ(id, 15);
  ASSERT_EQ(id/5, 3);
  ASSERT_EQ(id, 15);
}

TEST(chord_uuid, operator_equals) {
  ASSERT_TRUE(uuid_t{10}==uuid_t{10});
  ASSERT_FALSE(uuid_t{0}==uuid_t{10});
}

TEST(chord_uuid, operator_not_equals) {
  ASSERT_FALSE(uuid_t{10}!=uuid_t{10});
  ASSERT_TRUE(uuid_t{0}!=uuid_t{10});
}

TEST(chord_uuid, operator_geq) {
  ASSERT_TRUE(uuid_t{10} >= uuid_t{10});
  ASSERT_TRUE(uuid_t{20} >= uuid_t{10});
  ASSERT_FALSE(uuid_t{0} >= uuid_t{10});
}

TEST(chord_uuid, operator_leq) {
  ASSERT_TRUE(uuid_t{10} <= uuid_t{10});
  ASSERT_TRUE(uuid_t{0} <= uuid_t{10});
  ASSERT_FALSE(uuid_t{20} <= uuid_t{10});
}

TEST(chord_uuid, operator_less) {
  ASSERT_FALSE(uuid_t{10} < uuid_t{10});
  ASSERT_FALSE(uuid_t{20} < uuid_t{10});
  ASSERT_TRUE(uuid_t{0} < uuid_t{10});
}

TEST(chord_uuid, operator_greater) {
  ASSERT_FALSE(uuid_t{10} > uuid_t{10});
  ASSERT_FALSE(uuid_t{0} > uuid_t{10});
  ASSERT_TRUE(uuid_t{20} > uuid_t{10});
}

TEST(chord_uuid, operator_string_cast) {
  auto id = uuid_t{15};
  auto str = string{id};
  ASSERT_EQ(str, "15");
}

TEST(chord_uuid, operator_input_stream) {
  uuid_t id;
  stringstream input_stream;
  input_stream << "12345";
  input_stream >> id;
  ASSERT_EQ(id, 12345);
}
