#pragma once
#include "core/serializers/exception.hpp"

#include <type_traits>

namespace fetch {
namespace serializers {

template <typename T>
inline void Serialize(T &serializer, SerializableException const &s) {
  uint64_t size = s.explanation().size();
  error::error_type code = s.error_code();

  serializer.Allocate(sizeof(error::error_type) + sizeof(uint64_t) +
                      s.explanation().size());

  serializer.WriteBytes(reinterpret_cast<uint8_t const *>(&code),
                        sizeof(error::error_type));
  serializer.WriteBytes(reinterpret_cast<uint8_t const *>(&size),
                        sizeof(uint64_t));
  serializer.WriteBytes(
      reinterpret_cast<uint8_t const *>(s.explanation().c_str()), size);
}

template <typename T>
inline void Deserialize(T &serializer, SerializableException &s) {
  error::error_type code;
  uint64_t size = 0;

  serializer.ReadBytes(reinterpret_cast<uint8_t *>(&code),
                       sizeof(error::error_type));
  serializer.ReadBytes(reinterpret_cast<uint8_t *>(&size), sizeof(uint64_t));

  byte_array::ByteArray buffer;
  buffer.Resize(size);
  serializer.ReadBytes(buffer.pointer(), size);
  s = SerializableException(code, buffer);
}
}
}

