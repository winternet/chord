#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <iomanip>

#include "chord.crypto.h"
#include "chord.uuid.h"

#include <boost/multiprecision/cpp_int.hpp>
using namespace std;
using namespace chord;

using boost::multiprecision::import_bits;

TEST(CryptoTest, sha256) {
  auto data = string{"foobar"};

  auto str = crypto::sha256(data);
  auto ptr = crypto::sha256(data.data(), data.length());

  EXPECT_TRUE(str == ptr);

  auto expected = "C3AB8FF13720E8AD9047DD39466B3C8974E592C2FA383D4A3960714CAEF0C4F2"s;

}

TEST(CryptoTest, initialize) {
  unsigned char md[32];
  crypto::sha256("foo", 3, md);
  cout << "HASH: ";
  for(auto ch : md) printf("%X", ch);
  
  auto uid = uuid{0};
  import_bits(uid.value(), md, md+32);

  
  cout << "\nUUID: " << uid;
  cout << "\nUUID-HASH: " << std::hex << uid;
}
