#include <string>
#include <openssl/sha.h>

template<typename T>
class Hash {

public:
  T hash(const std::string& path) {
    // TODO check length
    T hash;

    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, path, path.size());
    SHA256_Final(hash, &sha256);

    return hash;
  }

};
