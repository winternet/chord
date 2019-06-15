#pragma once

#include <array>
#include <cctype>
#include <string>
#include <sstream>
#include <fstream>
#include <openssl/sha.h>
#include <boost/multiprecision/cpp_int.hpp>

#include "chord.uuid.h"
#include "chord.exception.h"

namespace chord {
namespace crypto {

using boost::multiprecision::import_bits;

inline void sha256(const void *input, unsigned long length, unsigned char *hash) {
  SHA256_CTX context;
  if (!SHA256_Init(&context))
    throw__exception("failed to initialize SHA256");

  if (!SHA256_Update(&context, static_cast<const unsigned char*>(input), length))
    throw__exception("failed to update SHA256");

  if (!SHA256_Final(hash, &context))
    throw__exception("failed to finalise SHA256");
}

inline uuid_t sha256(const void *input, unsigned long length) {
  auto uuid = uuid_t{0};
  unsigned char hash[SHA256_DIGEST_LENGTH];

  sha256(input, length, hash);
  import_bits(uuid.value(), hash, hash + SHA256_DIGEST_LENGTH);

  return uuid;
}

inline uuid_t sha256(const std::string &str) {
  return sha256(str.data(), str.length());
}

inline uuid_t sha256(std::istream &istream) {
  auto uuid = uuid_t{0};
  constexpr const std::size_t buffer_size { 1 << 12 };
  std::array<char, buffer_size> buffer;

  unsigned char hash[SHA256_DIGEST_LENGTH] = {0};

  SHA256_CTX context;
  if (!SHA256_Init(&context))
    throw__exception("failed to initialize SHA256");

  while(const auto read = istream.readsome(buffer.data(), buffer_size)) {
    if(!SHA256_Update(&context, buffer.data(), read))
      throw__exception("failed to update SHA256");
  }
  if(!SHA256_Final(hash, &context))
    throw__exception("failed to finalise SHA256");

  import_bits(uuid.value(), hash, hash + SHA256_DIGEST_LENGTH);
  return uuid;
}


}
}
