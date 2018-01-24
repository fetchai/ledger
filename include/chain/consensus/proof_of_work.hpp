#ifndef CHAIN_CONSENSUS_PROOF_OF_WORK_HPP
#define CHAIN_CONSENSUS_PROOF_OF_WORK_HPP
#include"crypto/sha256.hpp"
#include"crypto/bignumber.hpp"
#include"byte_array/const_byte_array.hpp"
namespace fetch {
namespace consensus {
  
class ProofOfWork : public crypto::BigUnsigned {
public:
  typedef crypto::BigUnsigned super_type; 

  ProofOfWork() = default;
  ProofOfWork(byte_array::ConstByteArray header) {
    header_ = header;
  }
  
  
  bool operator()() {
    crypto::SHA256 hasher;
    hasher.Reset();
    hasher.Update( header_ );
    hasher.Update( *this );
    digest_ = hasher.digest();
    hasher.Reset();
    hasher.Update( digest_ );
    digest_ = hasher.digest();

    return digest_ < target_;
  }

  void SetTarget( std::size_t zeros ) {
    target_ = 1;
    target_ <<= 8*sizeof(uint8_t)*super_type::size() - 1 - zeros;
  }

  void SetHeader(  byte_array::ConstByteArray header) {
    header_ = header;
  }
  
  
  crypto::BigUnsigned digest() const { return digest_; }
  crypto::BigUnsigned target() const { return target_; }  
private:
  crypto::BigUnsigned digest_;
  crypto::BigUnsigned target_;
  byte_array::ConstByteArray header_;
};
  
};
};

#endif
