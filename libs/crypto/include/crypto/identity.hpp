#ifndef CRYPTO_IDENTITY_HPP
#define CRYPTO_IDNETITY_HPP
#include "core/byte_array/byte_array.hpp"
namespace fetch {
namespace crypto {



class Identity {
public:
  Identity(byte_array::ConstByteArray identity_paramters, byte_array::ConstByteArray identifier) :
    identity_paramters_(identity_paramters.Copy()), identifier_(identifier.Copy()) 
  { }
  
  Identity(Identity const & other) :
    identity_paramters_(other.identity_paramters_), identifier_(other.identifier) 
  { }
  
  byte_array::ConstByteArray const &identity_paramters() {
    return identity_paramters_;
  }
  
  byte_array::ConstByteArray const &identifier() 
  {
    return identifier_; 
  }
  
private:
  byte_array::ConstByteArray identity_paramters_;
  byte_array::ConstByteArray identifier_;  
};


}
}
