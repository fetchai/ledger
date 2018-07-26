#pragma once
#include "crypto/identity.hpp"

namespace fetch {
namespace crypto {
class Verifier
{
public:
  typedef byte_array::ConstByteArray byte_array_type;

  virtual Identity identity()                               = 0;
  virtual bool     Verify(byte_array_type const &data,
                          byte_array_type const &signature) = 0;
};
}  // namespace crypto
}  // namespace fetch

