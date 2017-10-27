#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <iomanip>
#include <fstream>

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

  EXPECT_EQ(str, ptr);

  stringstream actual; actual << std::hex << str;
  auto expected = "c3ab8ff13720e8ad9047dd39466b3c8974e592c2fa383d4a3960714caef0c4f2"s;

  EXPECT_EQ(actual.str(), expected);
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

TEST(CryptoTest, hash_file) {

  {
  std::fstream file;
  file.open("test.txt", std::fstream::out | std::fstream::trunc | std::fstream::binary);
  file << "SOMECONTENT";
  file.flush();
  file.close();
  }

  std::fstream file;
  file.open("test.txt", std::fstream::in | std::fstream::app | std::fstream::binary);
  uuid_t hash = crypto::sha256(file);
  file.close();
  
  cout << "\n--UUID: " << hash;
  cout << "\n--UUID-HASH: " << std::nouppercase << std::hex << hash;

  ASSERT_EQ(hash.hex(), "accf25d1f41665e077c819907458c7363f30083c223cd3718ec851249ab647f9");

}
