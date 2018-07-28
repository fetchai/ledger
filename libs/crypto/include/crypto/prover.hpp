#pragma once

#include "core/byte_array/byte_array.hpp"
#include "crypto/identity.hpp"

namespace fetch {
namespace crypto {

class Prover
{
public:
  using byte_array_type = byte_array::ByteArray;

  virtual Identity identity() = 0;
  virtual ~Prover() {}

  virtual void            Load(byte_array_type const &)     = 0;
  virtual bool            Sign(byte_array_type const &text) = 0;
  virtual byte_array_type document_hash()                   = 0;
  virtual byte_array_type signature()                       = 0;
};

}  // namespace crypto
}  // namespace fetch
