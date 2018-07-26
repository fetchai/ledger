#ifndef CRYPTO_HASH_HPP
#define CRYPTO_HASH_HPP
#include "core/byte_array/byte_array.hpp"

namespace fetch {
namespace crypto {
template <typename T>
byte_array::ByteArray Hash(byte_array::ConstByteArray const &str)
{
  T hasher;

  hasher.Reset();
  hasher.Update(str);
  hasher.Final();

  return hasher.digest();
}
}  // namespace crypto
}  // namespace fetch

#endif
