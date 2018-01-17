#ifndef SERIALIZER_STIL_TYPES_HPP
#define SERIALIZER_STIL_TYPES_HPP
#include "byte_array/referenced_byte_array.hpp"
#include "assert.hpp"

#include <string>
#include <type_traits>
namespace fetch {
namespace serializers {

template <typename T, typename U>
typename std::enable_if< std::is_integral< U >::value, void >::type Serialize(T &serializer, U const &val) {  
  serializer.Allocate(sizeof(U));
  serializer.WriteBytes(reinterpret_cast<uint8_t const *>(&val),
                        sizeof(U));
}

template <typename T, typename U>
typename std::enable_if< std::is_integral< U >::value, void >::type  Deserialize(T &serializer, U &val) {
  detailed_assert( sizeof(U) <= serializer.bytes_left());
  serializer.ReadBytes(reinterpret_cast<uint8_t *>(&val), sizeof(U));
}

// Byte_Array
template <typename T>
void Serialize(T &serializer, std::string const &s) {
  serializer.Allocate(sizeof(uint64_t) + s.size());
  uint64_t size = s.size();

  serializer.WriteBytes(reinterpret_cast<uint8_t const *>(&size),
                        sizeof(uint64_t));
  serializer.WriteBytes(reinterpret_cast<uint8_t const *>(s.data()), s.size());
}

template <typename T>
void Deserialize(T &serializer, std::string &s) {
  uint64_t size = 0;

  detailed_assert( sizeof(uint64_t) <= serializer.bytes_left());
  serializer.ReadBytes(reinterpret_cast<uint8_t *>(&size), sizeof(uint64_t));
  detailed_assert( size <= serializer.bytes_left());  


  s.resize(size);
  char *buffer = new char[size];
  serializer.ReadBytes(reinterpret_cast<uint8_t *>(buffer), s.size());
  for (std::size_t i = 0; i < size; ++i) s[i] = buffer[i];
  delete[] buffer;
}

template <typename T>
void Serialize(T &serializer, char const *s) {
  return Serialize<T>(serializer, std::string(s));
}
};
};

#endif
