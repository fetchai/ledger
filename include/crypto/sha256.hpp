#ifndef CRYPTO_SHA256_HPP
#define CRYPTO_SHA256_HPP
#include "byte_array/referenced_byte_array.hpp"
#include "crypto/stream_hasher.hpp"

#include <openssl/sha.h>
#include <stdexcept>

namespace fetch {
namespace crypto {

class SHA256 : public StreamHasher {
 public:
  typedef byte_array::ReferencedByteArray byte_array_type;

  void Reset() override {
    if (!SHA256_Init(&data_))
      throw std::runtime_error("could not intialialise SHA256.");
  }

  bool Update(byte_array_type const& s) override {
    if (!SHA256_Update(&data_, s.pointer(), s.size())) return false;
    return true;
  }

  byte_array_type digest() override {
    byte_array_type ret;
    ret.Resize(SHA256_DIGEST_LENGTH);
    uint8_t* p = reinterpret_cast<uint8_t*>(ret.pointer());
    if (!SHA256_Final(p, &data_))
      throw std::runtime_error("could not finalize SHA256.");

    return ret;
  }

 private:
  SHA256_CTX data_;
};
};
};

#endif
