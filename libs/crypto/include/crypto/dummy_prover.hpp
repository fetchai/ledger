#ifndef CRYPTO_DUMMY_PROVER_HPP
#define CRYPTO_DUMMY_PROVER_HPP
#include "core/byte_array/byte_array.hpp"
namespace fetch {
namespace crypto {

class DummyProver {
public:
  typedef byte_array::ByteArray byte_array_type;

  void LoadParameters(byte_array_type const &params) override
  {
    
  }    
  
  void StoreParameters(byte_array_type &params) const override
  {
    
  }    
  
  void Load(byte_array_type const &) override 
  {
    
  }
  
  bool Sign(byte_array_type const &text) override
  {

  }
  
    
  byte_array_type document_hash() override 
  {

  }
  
  byte_array_type signature()  override 
  {

  }
  
};
}
}

#endif
