#ifndef CRYPTO_SHA256_HPP
#define CRYPTO_SHA256_HPP
#include "core/byte_array/byte_array.hpp"
#include "crypto/stream_hasher.hpp"

#include <openssl/sha.h>
#include <stdexcept>

namespace fetch {
namespace crypto {

class SHA256 : public StreamHasher {
 public:
  typedef typename StreamHasher::byte_array_type byte_array_type;

  void Reset() override {
    digest_.Resize(0);
    if (!SHA256_Init(&data_))
      throw std::runtime_error("could not intialialise SHA256.");
  }

  bool Update(byte_array_type const& s) override {
    return Update(s.pointer(), s.size());
  }

  void Final() override {
    digest_.Resize(SHA256_DIGEST_LENGTH);
    Final(reinterpret_cast<uint8_t*>(digest_.pointer()));
  }

  bool Update(uint8_t const *p, std::size_t const &size) override {
    if (!SHA256_Update(&data_, p, size)) return false;
    return true;
  }

  void Final(uint8_t* p) override {
    if (!SHA256_Final(p, &data_))
      throw std::runtime_error("could not finalize SHA256.");
  }

  
  byte_array_type digest() override {
    assert(digest_.size() == SHA256_DIGEST_LENGTH);
    return digest_;
  }

 private:
  byte_array_type digest_;
  SHA256_CTX data_;
};
}
}

#endif
