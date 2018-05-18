#ifndef SERIALIZER_STIL_TYPES_HPP
#define SERIALIZER_STIL_TYPES_HPP

#include <string>
#include <utility>
#include <type_traits>
#include <vector>

#include "assert.hpp"
#include "byte_array/referenced_byte_array.hpp"
#include "logger.hpp"
namespace fetch {
namespace serializers {

template <typename T, typename U>
inline typename std::enable_if<std::is_integral<U>::value, void>::type
Serialize(T &serializer, U const &val) {
  serializer.Allocate(sizeof(U));
  serializer.WriteBytes(reinterpret_cast<uint8_t const *>(&val), sizeof(U));
}

template <typename T, typename U>
inline typename std::enable_if<std::is_integral<U>::value, void>::type
Deserialize(T &serializer, U &val) {
  //  detailed_assert( sizeof(U) <= serializer.bytes_left());
  serializer.ReadBytes(reinterpret_cast<uint8_t *>(&val), sizeof(U));
}

template <typename T, typename U>
inline typename std::enable_if<std::is_floating_point<U>::value, void>::type
Serialize(T &serializer, U const &val) {
  serializer.Allocate(sizeof(U));
  serializer.WriteBytes(reinterpret_cast<uint8_t const *>(&val), sizeof(U));
}

template <typename T, typename U>
inline typename std::enable_if<std::is_floating_point<U>::value, void>::type
Deserialize(T &serializer, U &val) {
  //  detailed_assert( sizeof(U) <= serializer.bytes_left());
  serializer.ReadBytes(reinterpret_cast<uint8_t *>(&val), sizeof(U));
}

// Byte_Array
template <typename T>
inline void Serialize(T &serializer, std::string const &s) {
  serializer.Allocate(sizeof(uint64_t) + s.size());
  uint64_t size = s.size();

  serializer.WriteBytes(reinterpret_cast<uint8_t const *>(&size),
                        sizeof(uint64_t));
  serializer.WriteBytes(reinterpret_cast<uint8_t const *>(s.data()), s.size());
}

template <typename T>
inline void Deserialize(T &serializer, std::string &s) {
  uint64_t size = 0;

  //  detailed_assert( sizeof(uint64_t) <= serializer.bytes_left());
  serializer.ReadBytes(reinterpret_cast<uint8_t *>(&size), sizeof(uint64_t));
  //  detailed_assert( size <= serializer.bytes_left());

  s.resize(size);
  char *buffer = new char[size];
  serializer.ReadBytes(reinterpret_cast<uint8_t *>(buffer), s.size());
  for (std::size_t i = 0; i < size; ++i) s[i] = buffer[i];
  delete[] buffer;
}

template <typename T>
inline void Serialize(T &serializer, char const *s) {
  return Serialize<T>(serializer, std::string(s));
}

template <typename T, typename U>
inline void Serialize(T &serializer, std::vector<U> const &vec) {
  // Allocating memory for the size
  serializer.Allocate(sizeof(uint64_t));

  uint64_t size = vec.size();
  // Writing the size to the byte array
  serializer.WriteBytes(reinterpret_cast<uint8_t const *>(&size),
                        sizeof(uint64_t));

  for (auto const &a : vec) serializer << a;
}

template <typename T, typename U>
inline void Deserialize(T &serializer, std::vector<U> &vec) {
  uint64_t size;
  // Writing the size to the byte array
  serializer.ReadBytes(reinterpret_cast<uint8_t *>(&size), sizeof(uint64_t));

  // Reading the data
  vec.clear();
  vec.resize(size);

  for (auto &a : vec) serializer >> a;
}

template <typename T, typename fir, typename sec>
inline void Serialize(T &serializer, std::pair<fir, sec> const &pair) {
  serializer << pair.first;
  serializer << pair.second;
}

template <typename T, typename fir, typename sec>
inline void Deserialize(T &serializer, std::pair<fir, sec> &pair) {
  fir first;
  sec second;
  serializer >> first;
  serializer >> second;
  pair = make_pair(first, second);
}
}
}

#endif
