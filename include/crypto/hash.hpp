#ifndef CRYPTO_HASH_HPP
#define CRYPTO_HASH_HPP
#include "byte_array/referenced_byte_array.hpp"

namespace fetch {
namespace crypto {
template <typename T>
byte_array::ReferencedByteArray Hash(
    byte_array::ReferencedByteArray const &str) {
  T hasher;
  hasher.Reset();
  hasher.Update(str);
  return hasher.digest();
};
};
};

#endif
