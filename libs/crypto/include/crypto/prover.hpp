#ifndef CRYPTO_PROVER_HPP
#define CRYPTO_PROVER_HPP
#include "core/byte_array/referenced_byte_array.hpp"
namespace fetch {
namespace crypto {
class Prover {
 public:
  typedef byte_array::ByteArray byte_array_type;

  virtual void Load(byte_array_type const &) = 0;
  virtual bool Sign(byte_array_type const &text) = 0;
  virtual byte_array_type document_hash() = 0;
  virtual byte_array_type signature() = 0;
};
}
}

#endif
