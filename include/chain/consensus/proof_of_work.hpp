#ifndef CHAIN_CONSENSUS_PROOF_OF_WORK_HPP
#define CHAIN_CONSENSUS_PROOF_OF_WORK_HPP
#include"crypto/sha256.hpp"
#include"crypto/bignumber.hpp"
#include"byte_array/const_byte_array.hpp"
namespace fetch {
namespace chain {
namespace consensus {
  
class ProofOfWork : public crypto::BigUnsigned {
public:
  typedef crypto::BigUnsigned super_type; 
  typedef byte_array::ConstByteArray header_type;
  
  ProofOfWork() = default;
  ProofOfWork(header_type header) {
    header_ = header;
  }
  
  
  bool operator()() {
    crypto::SHA256 hasher;
    hasher.Reset();
    hasher.Update( header_ );
    hasher.Update( *this );
    hasher.Final();
    digest_ = hasher.digest();
    hasher.Reset();
    hasher.Update( digest_ );
    hasher.Final();
    
    digest_ = hasher.digest();

    return digest_ < target_;
  }

  void SetTarget( std::size_t zeros ) {
    target_ = 1;
    target_ <<= 8*sizeof(uint8_t)*super_type::size() - 1 - zeros;
  }

  void SetHeader(byte_array::ByteArray header) {
    header_ = header;
    assert( header_ == header );
  }
  
  header_type const& header() const { return header_; }  
  crypto::BigUnsigned digest() const { return digest_; }
  crypto::BigUnsigned target() const { return target_; }

  double set_accumulated_work(double const &w) { return  accumulated_work_ = w; }
  double const& accumulated_work() const { return  accumulated_work_; }  
  
private:
  crypto::BigUnsigned digest_;
  crypto::BigUnsigned target_;
  header_type header_;
  double accumulated_work_;  
};
  
};
};
};
#endif
