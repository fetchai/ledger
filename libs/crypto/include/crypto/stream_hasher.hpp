#pragma once
#include "core/byte_array/byte_array.hpp"

namespace fetch {
namespace crypto {
class StreamHasher
{
public:
  typedef byte_array::ByteArray byte_array_type;

  virtual void            Reset()                                           = 0;
  virtual bool            Update(byte_array_type const &data)               = 0;
  virtual bool            Update(uint8_t const *p, std::size_t const &size) = 0;
  virtual void            Final()                                           = 0;
  virtual void            Final(uint8_t *p)                                 = 0;
  virtual byte_array_type digest()                                          = 0;
};
}  // namespace crypto
}  // namespace fetch
