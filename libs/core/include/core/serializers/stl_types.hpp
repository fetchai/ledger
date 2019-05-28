#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018-2019 Fetch.AI Limited
//
//   Licensed under the Apache License, Version 2.0 (the "License");
//   you may not use this file except in compliance with the License.
//   You may obtain a copy of the License at
//
//       http://www.apache.org/licenses/LICENSE-2.0
//
//   Unless required by applicable law or agreed to in writing, software
//   distributed under the License is distributed on an "AS IS" BASIS,
//   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//   See the License for the specific language governing permissions and
//   limitations under the License.
//
//------------------------------------------------------------------------------

#include <array>
#include <map>
#include <set>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "core/assert.hpp"
#include "core/byte_array/byte_array.hpp"
#include "core/logger.hpp"

namespace fetch {
namespace serializers {

template <typename T, typename U>
inline typename std::enable_if<std::is_integral<U>::value, void>::type Serialize(T &serializer,
                                                                                 U const &val)
{
  serializer.Allocate(sizeof(U));
  serializer.WriteBytes(reinterpret_cast<uint8_t const *>(&val), sizeof(U));
}

template <typename T, typename U>
inline typename std::enable_if<std::is_integral<U>::value, void>::type Deserialize(T &serializer,
                                                                                   U &val)
{
  //  detailed_assert( sizeof(U) <= serializer.bytes_left());
  serializer.ReadBytes(reinterpret_cast<uint8_t *>(&val), sizeof(U));
}

template <typename T, typename U>
inline typename std::enable_if<std::is_floating_point<U>::value, void>::type Serialize(
    T &serializer, U const &val)
{
  serializer.Allocate(sizeof(U));
  serializer.WriteBytes(reinterpret_cast<uint8_t const *>(&val), sizeof(U));
}

template <typename T, typename U>
inline typename std::enable_if<std::is_floating_point<U>::value, void>::type Deserialize(
    T &serializer, U &val)
{
  //  detailed_assert( sizeof(U) <= serializer.bytes_left());
  serializer.ReadBytes(reinterpret_cast<uint8_t *>(&val), sizeof(U));
}

// Byte_Array
template <typename T>
inline void Serialize(T &serializer, std::string const &s)
{
  serializer.Allocate(sizeof(uint64_t) + s.size());
  uint64_t size = s.size();

  serializer.WriteBytes(reinterpret_cast<uint8_t const *>(&size), sizeof(uint64_t));
  serializer.WriteBytes(reinterpret_cast<uint8_t const *>(s.data()), s.size());
}

template <typename T>
inline void Deserialize(T &serializer, std::string &s)
{
  uint64_t size = 0;

  //  detailed_assert( sizeof(uint64_t) <= serializer.bytes_left());
  serializer.ReadBytes(reinterpret_cast<uint8_t *>(&size), sizeof(uint64_t));
  //  detailed_assert( size <= serializer.bytes_left());

  s.resize(size);
  char *buffer = new char[size];
  serializer.ReadBytes(reinterpret_cast<uint8_t *>(buffer), s.size());
  for (std::size_t i = 0; i < size; ++i)
  {
    s[i] = buffer[i];
  }
  delete[] buffer;
}

template <typename T>
inline void Serialize(T &serializer, char const *s)
{
  return Serialize<T>(serializer, std::string(s));
}

template <typename T, typename U, std::size_t N>
inline typename std::enable_if<std::is_integral<U>::value>::type Serialize(
    T &serializer, std::array<U, N> const &val)
{
  static constexpr std::size_t BINARY_SIZE = sizeof(U) * N;
  assert(N == val.size());

  serializer.Allocate(BINARY_SIZE);
  serializer.WriteBytes(reinterpret_cast<uint8_t const *>(val.data()), BINARY_SIZE);
}

template <typename T, typename U, std::size_t N>
inline typename std::enable_if<std::is_integral<U>::value>::type Deserialize(T &serializer,
                                                                             std::array<U, N> &val)
{
  static constexpr std::size_t BINARY_SIZE = sizeof(U) * N;
  assert(N == val.size());

  serializer.ReadBytes(reinterpret_cast<uint8_t *>(val.data()), BINARY_SIZE);
}

template <typename T, typename U>
inline void Serialize(T &serializer, std::vector<U> const &vec)
{
  // Allocating memory for the size
  serializer.Allocate(sizeof(uint64_t));

  uint64_t size = vec.size();
  // Writing the size to the byte array
  serializer.WriteBytes(reinterpret_cast<uint8_t const *>(&size), sizeof(uint64_t));

  for (auto const &a : vec)
  {
    serializer << a;
  }
}

template <typename T, typename U>
inline void Deserialize(T &serializer, std::vector<U> &vec)
{
  uint64_t size;
  // Writing the size to the byte array
  serializer.ReadBytes(reinterpret_cast<uint8_t *>(&size), sizeof(uint64_t));

  // Reading the data
  vec.clear();
  vec.resize(size);

  for (auto &a : vec)
  {
    serializer >> a;
  }
}

template <typename T, typename fir, typename sec>
inline void Serialize(T &serializer, std::pair<fir, sec> const &pair)
{
  serializer << pair.first;
  serializer << pair.second;
}

template <typename T, typename fir, typename sec>
inline void Deserialize(T &serializer, std::pair<fir, sec> &pair)
{
  fir first;
  sec second;
  serializer >> first;
  serializer >> second;
  pair = std::make_pair(std::move(first), std::move(second));
}

template <typename T, typename K, typename V, typename H, typename E>
inline void Serialize(T &serializer, std::unordered_map<K, V, H, E> const &map)
{

  // Allocating memory for the size
  serializer.Allocate(sizeof(uint64_t));

  uint64_t size = map.size();

  // Writing the size to the byte array
  serializer.WriteBytes(reinterpret_cast<uint8_t const *>(&size), sizeof(uint64_t));

  for (auto const &element : map)
  {
    serializer << element.first << element.second;
  }
}

template <typename T, typename K, typename V, typename H, typename E>
inline void Deserialize(T &serializer, std::unordered_map<K, V, H, E> &map)
{

  // Read the number of items in the map
  uint64_t size{0};
  serializer.ReadBytes(reinterpret_cast<uint8_t *>(&size), sizeof(uint64_t));

  // Reset the map
  map.clear();

  // Update the map
  K key{};
  V value{};
  for (uint64_t i = 0; i < size; ++i)
  {
    serializer >> key;
    serializer >> value;

    map[key] = value;
  }
}

template <typename T, typename K, typename V>
inline void Serialize(T &serializer, std::map<K, V> const &map)
{
  // Allocating memory for the size
  serializer.Allocate(sizeof(uint64_t));

  uint64_t size = map.size();

  // Writing the size to the byte array
  serializer.WriteBytes(reinterpret_cast<uint8_t const *>(&size), sizeof(uint64_t));

  for (auto const &element : map)
  {
    serializer << element.first << element.second;
  }
}

template <typename T, typename K, typename V>
inline void Deserialize(T &serializer, std::map<K, V> &map)
{

  // Read the number of items in the map
  uint64_t size{0};
  serializer.ReadBytes(reinterpret_cast<uint8_t *>(&size), sizeof(uint64_t));

  // Reset the map
  map.clear();

  // Update the map
  K key{};
  V value{};
  for (uint64_t i = 0; i < size; ++i)
  {
    serializer >> key;
    serializer >> value;

    map[key] = value;
  }
}

template <typename T, typename K, typename H>
inline void Serialize(T &serializer, std::unordered_set<K, H> const &set)
{

  // Allocating memory for the size
  serializer.Allocate(sizeof(uint64_t));

  uint64_t size = set.size();

  // Writing the size to the byte array
  serializer.WriteBytes(reinterpret_cast<uint8_t const *>(&size), sizeof(uint64_t));

  for (auto const &element : set)
  {
    serializer << element;
  }
}

template <typename T, typename K, typename H>
inline void Deserialize(T &serializer, std::unordered_set<K, H> &set)
{

  // Read the number of items in the map
  uint64_t size{0};
  serializer.ReadBytes(reinterpret_cast<uint8_t *>(&size), sizeof(uint64_t));

  // Reset the map
  set.clear();

  // Update the map
  K key{};
  for (uint64_t i = 0; i < size; ++i)
  {
    serializer >> key;
    set.insert(key);
  }
}

template <typename T, typename K>
inline void Serialize(T &serializer, std::set<K> const &set)
{

  // Allocating memory for the size
  serializer.Allocate(sizeof(uint64_t));

  uint64_t size = set.size();

  // Writing the size to the byte array
  serializer.WriteBytes(reinterpret_cast<uint8_t const *>(&size), sizeof(uint64_t));

  for (auto const &element : set)
  {
    serializer << element;
  }
}

template <typename T, typename K>
inline void Deserialize(T &serializer, std::set<K> &set)
{

  // Read the number of items in the map
  uint64_t size{0};
  serializer.ReadBytes(reinterpret_cast<uint8_t *>(&size), sizeof(uint64_t));

  // Reset the map
  set.clear();

  // Update the map
  K key{};
  for (uint64_t i = 0; i < size; ++i)
  {
    serializer >> key;
    set.insert(key);
  }
}

}  // namespace serializers
}  // namespace fetch
