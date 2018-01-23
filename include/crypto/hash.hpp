#ifndef CRYPTO_HASH_HPP
#define CRYPTO_HASH_HPP
#include "byte_array/const_byte_array.hpp"

namespace fetch {
namespace crypto {
template <typename T>
byte_array::ByteArray Hash(
    byte_array::ConstByteArray const &str) {
  T hasher;
  hasher.Reset();
  hasher.Update(str);
  return hasher.digest();
};
};
};

#endif
