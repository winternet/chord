#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <iomanip>
#include <bitset>

#include "chord.crypto.h"
#include "chord.uuid.h"

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
  auto arr = array<int, 4>{1431655765,1431655765,1431655765,1431655765};  // 4xsizeof(int) = 16 byte
  auto uuid_str = uuid_t{"113427455640312821154458202477256070485"};
  auto uuid_ptr = uuid_t{arr.begin(), arr.end()};

  ASSERT_EQ(uuid_str, uuid_ptr);
}

TEST(chord_uuid, raw_pointer_constructor) {
  auto bits = bitset<256>{}; //256 bit / 8 bit / 4 = 8 byte
  for(int i=0; i<256; i++)
    if(i%2==0)
      bits.set(i, true);
    else
      bits.set(i, false);

  auto uuid_str = uuid_t{"38597363079105398474523661669562635951089994888546854679819194669304376546645"};
  auto uuid_ptr = uuid_t{&bits, &bits+8};
}
