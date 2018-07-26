#pragma once

#include <string>
#include <utility>
#include <type_traits>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <map>
#include <set>

#include "core/assert.hpp"
#include "core/byte_array/byte_array.hpp"
#include "core/logger.hpp"

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
  pair = std::make_pair(std::move(first), std::move(second));
}

template <typename T, typename K, typename V, typename H>
inline void Serialize(T &serializer, std::unordered_map<K,V,H> const &map) {

  // Allocating memory for the size
  serializer.Allocate(sizeof(uint64_t));

  uint64_t size = map.size();

  // Writing the size to the byte array
  serializer.WriteBytes(reinterpret_cast<uint8_t const *>(&size), sizeof(uint64_t));

  for (auto const &element : map) {
    serializer << element.first << element.second;
  }
}

template <typename T, typename K, typename V, typename H>
inline void Deserialize(T &serializer, std::unordered_map<K,V,H> &map) {

  // Read the number of items in the map
  uint64_t size{0};
  serializer.ReadBytes(reinterpret_cast<uint8_t *>(&size), sizeof(uint64_t));

  // Reset the map
  map.clear();

  // Update the map
  K key{};
  V value{};
  for (uint64_t i = 0; i < size; ++i) {
    serializer >> key;
    serializer >> value;

    map[key] = value;
  }
}

template <typename T, typename K, typename H>
inline void Serialize(T &serializer, std::unordered_set<K,H> const &set) {

  // Allocating memory for the size
  serializer.Allocate(sizeof(uint64_t));

  uint64_t size = set.size();

  // Writing the size to the byte array
  serializer.WriteBytes(reinterpret_cast<uint8_t const *>(&size), sizeof(uint64_t));

  for (auto const &element : set) {
    serializer << element;
  }
}

template <typename T, typename K, typename H>
inline void Deserialize(T &serializer, std::unordered_set<K,H> &set) {

  // Read the number of items in the map
  uint64_t size{0};
  serializer.ReadBytes(reinterpret_cast<uint8_t *>(&size), sizeof(uint64_t));

  // Reset the map
  set.clear();

  // Update the map
  K key{};
  for (uint64_t i = 0; i < size; ++i) {
    serializer >> key;
    set.insert(key);
  }
}

template <typename T, typename K>
inline void Serialize(T &serializer, std::set<K> const &set) {

  // Allocating memory for the size
  serializer.Allocate(sizeof(uint64_t));

  uint64_t size = set.size();

  // Writing the size to the byte array
  serializer.WriteBytes(reinterpret_cast<uint8_t const *>(&size), sizeof(uint64_t));

  for (auto const &element : set) {
    serializer << element;
  }
}

template <typename T, typename K>
inline void Deserialize(T &serializer, std::set<K> &set) {

  // Read the number of items in the map
  uint64_t size{0};
  serializer.ReadBytes(reinterpret_cast<uint8_t *>(&size), sizeof(uint64_t));

  // Reset the map
  set.clear();

  // Update the map
  K key{};
  for (uint64_t i = 0; i < size; ++i) {
    serializer >> key;
    set.insert(key);
  }
}

}
}

