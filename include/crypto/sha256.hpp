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
  typedef typename StreamHasher::byte_array_type  byte_array_type;

  void Reset() override {
    digest_.Resize(0);
    if (!SHA256_Init(&data_))
      throw std::runtime_error("could not intialialise SHA256.");
  }

  bool Update(byte_array_type const& s) override {
    if (!SHA256_Update(&data_, s.pointer(), s.size())) return false;
    return true;
  }

  void Final() override {
    digest_.Resize(SHA256_DIGEST_LENGTH);
    uint8_t* p = reinterpret_cast<uint8_t*>(digest_.pointer());
    if (!SHA256_Final(p, &data_))
      throw std::runtime_error("could not finalize SHA256.");
  }
  
  byte_array_type digest() override {
    assert( digest_.size() == SHA256_DIGEST_LENGTH);
    return digest_;
  }

 private:
  byte_array_type digest_;  
  SHA256_CTX data_;
};
}
}

#endif
