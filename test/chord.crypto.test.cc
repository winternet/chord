#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <fstream>

#include "chord.crypto.h"

using namespace std;
using namespace chord;

using boost::multiprecision::import_bits;

TEST(CryptoTest, sha256) {
  auto data = string{"foobar"};

  auto str = crypto::sha256(data);
  auto ptr = crypto::sha256(data.data(), data.length());

  EXPECT_EQ(str, ptr);

  stringstream actual;
  actual << std::hex << str;
  auto expected_hex = "c3ab8ff13720e8ad9047dd39466b3c8974e592c2fa383d4a3960714caef0c4f2"s;
  auto expected_num = "88504129774694501264414584963656750197241212201858038081545232447814102992114"s;

  EXPECT_EQ(actual.str(), expected_num);
}

TEST(CryptoTest, initialize) {
  unsigned char md[32];
  crypto::sha256("foo", 3, md);

  //for (auto ch : md) printf("%X", ch);

  auto uid = uuid{0};
  import_bits(uid.value(), md, md + 32);

  ASSERT_EQ(uid.hex(), "2c26b46b68ffc68ff99b453c1d30413413422d706483bfa0f98a5e886266e7ae");
  ASSERT_EQ(uid, "19970150736239713706088444570146546354146685096673408908105596072151101138862");
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

  ASSERT_EQ(hash.string(), "78163808323680042193722866647697615020714063641725196338206602615142164613113");
  ASSERT_EQ(hash.hex(), "accf25d1f41665e077c819907458c7363f30083c223cd3718ec851249ab647f9");
}

TEST(CryptoTest, sha256_object) {
  {
    std::fstream file;
    file.open("test.txt", std::fstream::out | std::fstream::trunc | std::fstream::binary);
    file << "SOMECONTENT";
    file.flush();
    file.close();
  }

  std::fstream file;
  file.open("test.txt", std::fstream::in | std::fstream::app | std::fstream::binary);

  crypto::sha256_hasher hasher;
  constexpr const std::size_t buffer_size { 1 << 12 };
  std::array<char, buffer_size> buffer;

  while(const auto read = file.readsome(buffer.data(), buffer_size)) {
    hasher(buffer.data(), read);
  }

  file.close();

  const auto hash = hasher.get();

  ASSERT_EQ(hash.string(), "78163808323680042193722866647697615020714063641725196338206602615142164613113");
  ASSERT_EQ(hash.hex(), "accf25d1f41665e077c819907458c7363f30083c223cd3718ec851249ab647f9");
}
