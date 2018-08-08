#pragma once
#include "crypto/identity.hpp"

namespace fetch {
namespace crypto {
class Verifier
{
public:
  using byte_array_type = byte_array::ConstByteArray;

  virtual Identity identity()                                                            = 0;
  virtual bool     Verify(byte_array_type const &data, byte_array_type const &signature) = 0;
};
}  // namespace crypto
}  // namespace fetch
