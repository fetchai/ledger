#ifndef SERIALIZER_STIL_TYPES_HPP
#define SERIALIZER_STIL_TYPES_HPP
#include "byte_array/referenced_byte_array.hpp"

#include <string>
#include <type_traits>
namespace fetch {
namespace serializers {

// 64-bit integers
template <typename T>
void Serialize(T &serializer, uint64_t const &val) {
  serializer.Allocate(sizeof(uint64_t));
  serializer.WriteBytes(reinterpret_cast<uint8_t const *>(&val),
                        sizeof(uint64_t));
}

template <typename T>
void Deserialize(T &serializer, uint64_t &val) {
  serializer.ReadBytes(reinterpret_cast<uint8_t *>(&val), sizeof(uint64_t));
}

template <typename T>
void Serialize(T &serializer, int64_t const &val) {
  serializer.Allocate(sizeof(int64_t));
  serializer.WriteBytes(reinterpret_cast<uint8_t const *>(&val),
                        sizeof(int64_t));
}

template <typename T>
void Deserialize(T &serializer, int64_t &val) {
  serializer.ReadBytes(reinterpret_cast<uint8_t *>(&val), sizeof(int64_t));
}

// 32-bit integers
template <typename T>
void Serialize(T &serializer, uint32_t const &val) {
  serializer.Allocate(sizeof(uint32_t));
  serializer.WriteBytes(reinterpret_cast<uint8_t const *>(&val),
                        sizeof(uint32_t));
}

template <typename T>
void Deserialize(T &serializer, uint32_t &val) {
  serializer.ReadBytes(reinterpret_cast<uint8_t *>(&val), sizeof(uint32_t));
}

template <typename T>
void Serialize(T &serializer, int32_t const &val) {
  serializer.Allocate(sizeof(int32_t));
  serializer.WriteBytes(reinterpret_cast<uint8_t const *>(&val),
                        sizeof(int32_t));
}

template <typename T>
void Deserialize(T &serializer, int32_t &val) {
  serializer.ReadBytes(reinterpret_cast<uint8_t *>(&val), sizeof(int32_t));
}

// 16-bit integers
template <typename T>
void Serialize(T &serializer, uint16_t const &val) {
  serializer.Allocate(sizeof(uint16_t));
  serializer.WriteBytes(reinterpret_cast<uint8_t const *>(&val),
                        sizeof(uint16_t));
}

template <typename T>
void Deserialize(T &serializer, uint16_t &val) {
  serializer.ReadBytes(reinterpret_cast<uint8_t *>(&val), sizeof(uint16_t));
}

template <typename T>
void Serialize(T &serializer, int16_t const &val) {
  serializer.Allocate(sizeof(int16_t));
  serializer.WriteBytes(reinterpret_cast<uint8_t const *>(&val),
                        sizeof(int16_t));
}

template <typename T>
void Deserialize(T &serializer, int16_t &val) {
  serializer.ReadBytes(reinterpret_cast<uint8_t *>(&val), sizeof(int16_t));
}

// 8-bit integers
template <typename T>
void Serialize(T &serializer, uint8_t const &val) {
  serializer.Allocate(sizeof(uint8_t));
  serializer.WriteBytes(reinterpret_cast<uint8_t const *>(&val),
                        sizeof(uint8_t));
}

template <typename T>
void Deserialize(T &serializer, uint8_t &val) {
  serializer.ReadBytes(reinterpret_cast<uint8_t *>(&val), sizeof(uint8_t));
}

template <typename T>
void Serialize(T &serializer, int8_t const &val) {
  serializer.Allocate(sizeof(int8_t));
  serializer.WriteBytes(reinterpret_cast<uint8_t const *>(&val),
                        sizeof(int8_t));
}

template <typename T>
void Deserialize(T &serializer, int8_t &val) {
  serializer.ReadBytes(reinterpret_cast<uint8_t *>(&val), sizeof(int8_t));
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

  serializer.ReadBytes(reinterpret_cast<uint8_t *>(&size), sizeof(uint64_t));
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
