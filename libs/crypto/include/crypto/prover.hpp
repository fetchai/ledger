#ifndef CRYPTO_PROVER_HPP
#define CRYPTO_PROVER_HPP
#include "core/byte_array/byte_array.hpp"
namespace fetch {
namespace crypto {

class Prover {
 public:
  using byte_array_type = byte_array::ByteArray;

  virtual void Load(byte_array_type const &) = 0;
  virtual bool Sign(byte_array_type const &text) = 0;
  virtual byte_array_type document_hash() = 0;
  virtual byte_array_type signature() = 0;
  //virtual bool Sign(
  //        byte_array_type const &private_key,
  //        byte_array_type const &data_to_sign,
  //        byte_array_type &signature,
  //        byte_array_type &data_hash) = 0;
};

}
}

#endif

