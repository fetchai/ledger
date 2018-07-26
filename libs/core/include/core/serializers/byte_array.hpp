#pragma once
#include "core/assert.hpp"
#include "core/byte_array/byte_array.hpp"
#include "core/byte_array/const_byte_array.hpp"

#include <type_traits>

namespace fetch {
namespace serializers {

template <typename T>
inline void Serialize(T &serializer, byte_array::ConstByteArray const &s)
{
  serializer.Allocate(sizeof(uint64_t) + s.size());
  uint64_t size = s.size();

  serializer.WriteBytes(reinterpret_cast<uint8_t const *>(&size),
                        sizeof(uint64_t));
  serializer.WriteBytes(reinterpret_cast<uint8_t const *>(s.pointer()),
                        s.size());
}

template <typename T>
inline void Deserialize(T &serializer, byte_array::ConstByteArray &s)
{
  uint64_t size = 0;

  detailed_assert(int64_t(sizeof(uint64_t)) <= serializer.bytes_left());
  serializer.ReadBytes(reinterpret_cast<uint8_t *>(&size), sizeof(uint64_t));
  detailed_assert(int64_t(size) <= serializer.bytes_left());

  serializer.ReadByteArray(s, size);
  //  s.Resize(size);
  //  serializer.ReadBytes(reinterpret_cast<uint8_t *>(s.pointer()), s.size());
}
}  // namespace serializers
}  // namespace fetch

