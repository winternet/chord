#pragma once

#include <array>
#include <cctype>
#include <string>
#include <fstream>
#include <openssl/sha.h>
#include <openssl/evp.h>
#include <boost/multiprecision/cpp_int.hpp>

#include "chord.uuid.h"
#include "chord.path.h"
#include "chord.exception.h"

namespace chord {
namespace crypto {

using boost::multiprecision::import_bits;

struct sha256_hasher final {

  EVP_MD_CTX* context;
  const EVP_MD* digest;
  unsigned char hash[SHA256_DIGEST_LENGTH] = {0};

  sha256_hasher() {
    context = EVP_MD_CTX_create();
    digest = EVP_get_digestbyname("SHA256");
    if(!EVP_DigestInit_ex(context, digest, nullptr)) {
      throw__exception("failed to initialize SHA256");
    }
  }

  virtual ~sha256_hasher() {
    EVP_MD_CTX_destroy(context);
  }
  void operator()(const void* input, const unsigned long length) {
    update(input, length);
  }

  void update(const void* input, const unsigned long length) {
  if (!EVP_DigestUpdate(context, static_cast<const unsigned char*>(input), length))
    throw__exception("failed to update SHA256");
  }

  chord::uuid get() {
    if(!EVP_DigestFinal_ex(context, hash, nullptr))
      throw__exception("failed to finalise SHA256");
    auto uuid = uuid_t{0};
    import_bits(uuid.value(), hash, hash + SHA256_DIGEST_LENGTH);
    return uuid;
  }
};

inline void sha256(const void *input, unsigned long length, unsigned char *hash) {
  SHA256(static_cast<const unsigned char*>(input), length, hash);
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

  EVP_MD_CTX* context = EVP_MD_CTX_create();
  const EVP_MD* digest = EVP_get_digestbyname("SHA256");

  if (!EVP_DigestInit_ex(context, digest, nullptr))
    throw__exception("failed to initialize SHA256");

  while(const auto read = istream.readsome(buffer.data(), buffer_size)) {
    if(!EVP_DigestUpdate(context, buffer.data(), static_cast<size_t>(read)))
      throw__exception("failed to update SHA256");
  }
  if(!EVP_DigestFinal_ex(context, hash, nullptr))
    throw__exception("failed to finalise SHA256");

  EVP_MD_CTX_destroy(context);

  import_bits(uuid.value(), hash, hash + SHA256_DIGEST_LENGTH);
  return uuid;
}

inline uuid_t sha256(const chord::path& path) {
  std::ifstream istream(path);
  return sha256(istream);
}

}
}
