#ifndef CRYPTO_FNV_HPP
#define CRYPTO_FNV_HPP
#include "byte_array/referenced_byte_array.hpp"
#include "crypto/stream_hasher.hpp"

namespace fetch {
namespace crypto {

class FNV : public StreamHasher {
 public:
  typedef typename StreamHasher::byte_array_type byte_array_type;
  
  FNV()
  {
     digest_.Resize( 4 );
  }

  void Reset() override {
    context_ = 2166136261;  
  }

  bool Update(byte_array_type const& s) override {
    for (std::size_t i = 0; i < s.size(); ++i)
    {
        context_ = (context_ * 16777619) ^ s[i];
    }
    return true;
  }

  void Final() override {
     digest_[0] = context_;
     digest_[1] = context_ >> 8;
     digest_[2] = context_ >> 16;
     digest_[3] = context_ >> 24;      
  }
  
  byte_array_type digest() override {
    assert( digest_.size() == 4);
    return digest_;
  }

  uint32_t uint_digest() {
    return context_;
  }
  
private:
  uint32_t context_;
  byte_array_type digest_;  

};
};
};

#endif
