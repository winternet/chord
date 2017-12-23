#pragma once

#include <string>
#include <sstream>
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
    throw chord::exception("failed to initialize SHA256");

  if (!SHA256_Update(&context, (unsigned char *) input, length))
    throw chord::exception("failed to update SHA256");

  if (!SHA256_Final(hash, &context))
    throw chord::exception("failed to finalise SHA256");
}

inline uuid_t sha256(const void *input, unsigned long length) {
  auto uuid = uuid_t{0};
  unsigned char hash[32];

  sha256(input, length, hash);
  import_bits(uuid.value(), hash, hash + 32);

  return uuid;
}

inline uuid_t sha256(const std::string &str) {
  return sha256(str.data(), str.length());
}

inline uuid_t sha256(const std::istream &istream) {
  std::string str = dynamic_cast<std::stringstream const &>(std::stringstream() << istream.rdbuf()).str();
  return sha256(str.data(), str.length());
}

}
}
