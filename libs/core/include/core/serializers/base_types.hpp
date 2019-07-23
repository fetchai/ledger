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

#include "core/byte_array/byte_array.hpp"
#include "core/byte_array/const_byte_array.hpp"
#include "core/serializers/exception.hpp"
#include "core/serializers/group_definitions.hpp"
#include "vectorise/platform.hpp"

#include <array>
#include <map>
#include <set>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace fetch {
namespace serializers {
template <typename T, typename D, uint8_t C>
struct SignedIntegerSerializerImplementation
{
  using Type       = T;
  using DriverType = D;
  enum
  {
    CODE = C
  };

  template <typename Interface>
  static void Serialize(Interface &interface, Type const &val)
  {
    // TODO: Work out whether it is workwhile to make a
    // specialised small integer for speed.
    if (0 <= val && val < 128)
    {
      interface.Allocate(sizeof(uint8_t));
      interface.WriteByte(static_cast<uint8_t>(val));
    }
    // TODO: Support for small negative integers?
    else
    {
      interface.Allocate(sizeof(uint8_t) + sizeof(val));
      interface.WriteByte(CODE);
      interface.WriteBytes(reinterpret_cast<uint8_t const *>(&val), sizeof(val));
    }
  }

  template <typename Interface>
  static void Deserialize(Interface &interface, Type &val)
  {
    uint8_t code;
    interface.ReadByte(code);

    if ((code & (1 << 7)) == 0)
    {
      val = static_cast<Type>(code);
    }
    else if ((code & (7 << 5)) == (7 << 5))  // TODO: test
    {
      val = static_cast<Type>(code);
    }
    else
    {
      // TODO: Test code
      interface.ReadBytes(reinterpret_cast<uint8_t *>(&val), sizeof(val));
    }
  }
};

template <typename T, typename D, uint8_t C>
struct UnsignedIntegerSerializerImplementation
{
  using Type       = T;
  using DriverType = D;
  enum
  {
    CODE = C
  };

  template <typename Interface>
  static void Serialize(Interface &interface, Type const &val)
  {
    if (0 <= val && val < 128)
    {
      interface.Allocate(sizeof(uint8_t));
      interface.WriteByte(static_cast<uint8_t>(val));
    }
    else
    {
      interface.Allocate(sizeof(uint8_t) + sizeof(val));
      interface.WriteByte(CODE);
      interface.WriteBytes(reinterpret_cast<uint8_t const *>(&val), sizeof(val));
    }
  }

  template <typename Interface>
  static void Deserialize(Interface &interface, Type &val)
  {
    uint8_t code;
    interface.ReadByte(code);

    if ((code & (1 << 7)) == 0)
    {
      val = static_cast<Type>(code);
    }
    else if ((code & (7 << 5)) == (7 << 5))  // TODO: test
    {
      // TODO: Throw
      val = static_cast<Type>(code);
    }
    else
    {
      // TODO: Test code
      interface.ReadBytes(reinterpret_cast<uint8_t *>(&val), sizeof(val));
    }
  }
};

template <typename D>
struct IntegerSerializer<int8_t, D> : public SignedIntegerSerializerImplementation<int8_t, D, 0xd0>
{
};
template <typename D>
struct IntegerSerializer<int16_t, D>
  : public SignedIntegerSerializerImplementation<int16_t, D, 0xd1>
{
};
template <typename D>
struct IntegerSerializer<int32_t, D>
  : public SignedIntegerSerializerImplementation<int32_t, D, 0xd2>
{
};
template <typename D>
struct IntegerSerializer<int64_t, D>
  : public SignedIntegerSerializerImplementation<int64_t, D, 0xd4>
{
};

template <typename D>
struct IntegerSerializer<uint8_t, D>
  : public UnsignedIntegerSerializerImplementation<uint8_t, D, 0xcc>
{
};
template <typename D>
struct IntegerSerializer<uint16_t, D>
  : public UnsignedIntegerSerializerImplementation<uint16_t, D, 0xcd>
{
};
template <typename D>
struct IntegerSerializer<uint32_t, D>
  : public UnsignedIntegerSerializerImplementation<uint32_t, D, 0xce>
{
};
template <typename D>
struct IntegerSerializer<uint64_t, D>
  : public UnsignedIntegerSerializerImplementation<uint64_t, D, 0xcf>
{
};

template <typename T, typename D, uint8_t C>
struct SpecialTypeSerializer
{
  using Type       = T;
  using DriverType = D;
  enum
  {
    CODE = C
  };

  template <typename Interface>
  static void Serialize(Interface &interface)
  {
    interface.Allocate(sizeof(uint8_t));
    interface.WriteByte(CODE);
  }
};

template <typename D>
struct BooleanSerializer<std::true_type, D> : public SpecialTypeSerializer<std::true_type, D, 0xc3>
{
};

template <typename D>
struct BooleanSerializer<std::false_type, D>
  : public SpecialTypeSerializer<std::false_type, D, 0xc2>
{
};

template <typename D>
struct BooleanSerializer<bool, D>
{
  using DriverType = D;
  using Type       = bool;

  template <typename Interface>
  static void Serialize(Interface &interface, Type const &val)
  {
    if (val)
    {
      BooleanSerializer<std::true_type, D>::Serialize(interface);
    }
    else
    {
      BooleanSerializer<std::false_type, D>::Serialize(interface);
    }
  }

  template <typename Interface>
  static void Deserialize(Interface &interface, Type &val)
  {
    uint8_t code;
    interface.ReadByte(code);
    switch (code)
    {
    case BooleanSerializer<std::true_type, D>::CODE:
      val = true;
      break;
    case BooleanSerializer<std::false_type, D>::CODE:
      val = false;
      break;
    default:
      throw SerializableException(std::string("buffer type differs from expected type boolean"));
    }
  }
};

template <typename D>
struct FloatSerializer<float, D>
{
  using Type       = float;
  using DriverType = D;

  template <typename Interface>
  static void Serialize(Interface &interface, Type const &val)
  {
    uint8_t opcode = static_cast<uint8_t>(0xca);
    interface.Allocate(sizeof(opcode) + sizeof(val));
    interface.WriteBytes(&opcode, sizeof(opcode));
    interface.WriteBytes(reinterpret_cast<uint8_t const *>(&val), sizeof(val));
  }

  template <typename Interface>
  static void Deserialize(Interface &interface, Type &val)
  {
    uint8_t opcode;
    interface.ReadByte(opcode);
    if (opcode != static_cast<uint8_t>(0xca))
    {
      throw SerializableException(
          std::string("expected float for deserialisation, but other type found."));
    }
    interface.ReadBytes(reinterpret_cast<uint8_t *>(&val), sizeof(val));
  }
};

template <typename D>
struct FloatSerializer<double, D>
{
  using Type       = double;
  using DriverType = D;

  template <typename Interface>
  static void Serialize(Interface &interface, Type const &val)
  {
    uint8_t opcode = static_cast<uint8_t>(0xcb);
    interface.Allocate(sizeof(opcode) + sizeof(val));
    interface.WriteBytes(&opcode, sizeof(opcode));
    interface.WriteBytes(reinterpret_cast<uint8_t const *>(&val), sizeof(val));
  }

  template <typename Interface>
  static void Deserialize(Interface &interface, Type &val)
  {
    uint8_t opcode;
    interface.ReadByte(opcode);
    if (opcode != static_cast<uint8_t>(0xcb))
    {
      throw SerializableException(
          std::string("expected double for deserialisation, but other type found."));
    }
    interface.ReadBytes(reinterpret_cast<uint8_t *>(&val), sizeof(val));
  }
};

template <typename T>
struct StringPointerGetter
{
  static uint8_t const *GetPointer(T const &arr)
  {
    return reinterpret_cast<uint8_t const *>(arr.pointer());
  }
};

template <>
struct StringPointerGetter<std::string>
{
  static uint8_t const *GetPointer(std::string const &arr)
  {
    return reinterpret_cast<uint8_t const *>(arr.c_str());
  }
};

template <typename T, typename D>
struct StringSerializerImplementation
{
public:
  using Type       = T;
  using DriverType = D;
  enum
  {
    CODE_FIXED = 0xa0,
    CODE8      = 0xd9,
    CODE16     = 0xda,
    CODE32     = 0xdb
  };

  template <typename Interface>
  static void Serialize(Interface &interface, Type const &val)
  {
    // Serializing the size of the string
    uint8_t opcode;
    if (val.size() < 32)
    {
      opcode = static_cast<uint8_t>(CODE_FIXED | (val.size() & 0x1f));
      interface.Allocate(sizeof(opcode) + val.size());
      interface.WriteBytes(&opcode, sizeof(opcode));
    }
    else if (val.size() < (1 << 8))
    {
      opcode       = static_cast<uint8_t>(CODE8);
      uint8_t size = static_cast<uint8_t>(val.size());

      interface.Allocate(sizeof(opcode) + sizeof(size) + val.size());
      interface.WriteBytes(&opcode, sizeof(opcode));
      interface.WriteBytes(reinterpret_cast<uint8_t *>(&size), sizeof(size));
    }
    else if (val.size() < (1 << 16))
    {
      opcode        = static_cast<uint8_t>(CODE16);
      uint16_t size = static_cast<uint16_t>(val.size());

      interface.Allocate(sizeof(opcode) + sizeof(size) + val.size());
      interface.WriteBytes(&opcode, sizeof(opcode));
      interface.WriteBytes(reinterpret_cast<uint8_t *>(&size), sizeof(size));
    }
    else if (val.size() < (1ull << 32))
    {
      opcode        = static_cast<uint8_t>(CODE32);
      uint32_t size = static_cast<uint32_t>(val.size());
      interface.Allocate(sizeof(opcode) + sizeof(size) + val.size());

      interface.WriteBytes(&opcode, sizeof(opcode));
      interface.WriteBytes(reinterpret_cast<uint8_t *>(&size), sizeof(size));
    }
    else
    {
      throw SerializableException(
          error::TYPE_ERROR, std::string("Cannot create maps with more than 1 << 32 elements"));
    }
    // Serializing the payload
    interface.WriteBytes(StringPointerGetter<Type>::GetPointer(val), val.size());
  }

  template <typename Interface>
  static void Deserialize(Interface &interface, Type &val)
  {
    uint8_t  opcode;
    uint32_t size;
    interface.ReadByte(opcode);

    switch (opcode)
    {
    case CODE8:
    {
      uint8_t tmp;
      interface.ReadBytes(reinterpret_cast<uint8_t *>(&tmp), sizeof(uint8_t));
      size = static_cast<uint32_t>(tmp);
      break;
    }
    case CODE16:
    {
      uint8_t tmp;
      interface.ReadBytes(reinterpret_cast<uint8_t *>(&tmp), sizeof(uint16_t));
      size = static_cast<uint32_t>(tmp);
      break;
    }
    case CODE32:
      interface.ReadBytes(reinterpret_cast<uint8_t *>(&size), sizeof(size));
      break;
    default:  // Default CODE_FIXED
      if ((opcode & 0xE0) != CODE_FIXED)
      {
        // TODO: Change to serializable exception.
        throw std::runtime_error("expected CODE_FIXED in opcode: " + std::to_string(int(opcode)) + " vs " + std::to_string(int(CODE_FIXED)));
      }
      size = static_cast<uint32_t>(opcode & 0x1f);
    }
    byte_array::ByteArray arr;
    interface.ReadByteArray(arr, size);
    val = static_cast<Type>(arr);
  }
};

template <typename D>
struct StringSerializer<std::string, D> : public StringSerializerImplementation<std::string, D>
{
};
template <typename D>
struct StringSerializer<byte_array::ConstByteArray, D>
  : public StringSerializerImplementation<byte_array::ConstByteArray, D>
{
};
template <typename D>
struct StringSerializer<byte_array::ByteArray, D>
  : public StringSerializerImplementation<byte_array::ByteArray, D>
{
};

template <typename V, typename D>
struct ArraySerializer<std::vector<V>, D>
{
public:
  using Type       = std::vector<V>;
  using DriverType = D;

  template <typename Constructor>
  static void Serialize(Constructor &array_constructor, Type const &input)
  {
    auto array = array_constructor(input.size());

    for (auto &v : input)
    {
      array.Append(v);
    }
  }

  template <typename ArrayDeserializer>
  static void Deserialize(ArrayDeserializer &array, Type &output)
  {
    output.clear();
    output.resize(array.size());
    for (uint32_t i = 0; i < array.size(); ++i)
    {
      array.GetNextValue(output[i]);
    }
  }
};

template <typename V, typename D>
struct ArraySerializer<std::set<V>, D>
{
public:
  using Type       = std::set<V>;
  using DriverType = D;

  template <typename Constructor>
  static void Serialize(Constructor &array_constructor, Type const &input)
  {
    auto array = array_constructor(input.size());

    for (auto &v : input)
    {
      array.Append(v);
    }
  }

  template <typename ArrayDeserializer>
  static void Deserialize(ArrayDeserializer &array, Type &output)
  {
    output.clear();
    for (uint32_t i = 0; i < array.size(); ++i)
    {
      V v;
      array.GetNextValue(v);
      output.insert(std::move(v));
    }
  }
};

template <typename V, typename H, typename D>
struct ArraySerializer<std::unordered_set<V, H>, D>
{
public:
  using Type       = std::unordered_set<V, H>;
  using DriverType = D;

  template <typename Constructor>
  static void Serialize(Constructor &array_constructor, Type const &input)
  {
    auto array = array_constructor(input.size());

    for (auto &v : input)
    {
      array.Append(v);
    }
  }

  template <typename ArrayDeserializer>
  static void Deserialize(ArrayDeserializer &array, Type &output)
  {
    output.clear();
    for (uint32_t i = 0; i < array.size(); ++i)
    {
      V v;
      array.GetNextValue(v);
      output.insert(std::move(v));
    }
  }
};

template <typename V, std::size_t N, typename D>
struct ArraySerializer<std::array<V, N>, D>
{
public:
  using Type       = std::array<V, N>;
  using DriverType = D;

  template <typename Constructor>
  static void Serialize(Constructor &array_constructor, Type const &input)
  {
    auto array = array_constructor(N);
    for (auto &v : input)
    {
      array.Append(v);
    }
  }

  template <typename ArrayDeserializer>
  static void Deserialize(ArrayDeserializer &array, Type &output)
  {
    if (array.size() != N)
    {
      throw SerializableException(std::string("std::array size and deserialisable size differs."));
    }

    for (uint32_t i = 0; i < array.size(); ++i)
    {
      array.GetNextValue(output[i]);
    }
  }
};

template <typename K, typename V, typename H, typename E, typename D>
struct MapSerializer<std::unordered_map<K, V, H, E>, D>
{
public:
  using Type       = std::unordered_map<K, V, H, E>;
  using DriverType = D;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &input)
  {
    auto map = map_constructor(input.size());
    for (auto &v : input)
    {
      map.Append(v.first, v.second);
    }
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &output)
  {
    output.clear();
    for (uint64_t i = 0; i < map.size(); ++i)
    {
      K key;
      V value;
      map.GetNextKeyPair(key, value);
      output.insert({key, value});
    }
  }
};

template <typename K, typename V, typename D>
struct MapSerializer<std::map<K, V>, D>
{
public:
  using Type       = std::map<K, V>;
  using DriverType = D;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &input)
  {
    auto map = map_constructor(input.size());
    for (auto &v : input)
    {
      map.Append(v.first, v.second);
    }
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &output)
  {
    output.clear();
    for (uint64_t i = 0; i < map.size(); ++i)
    {
      K key;
      V value;
      map.GetNextKeyPair(key, value);
      output.insert({key, value});
    }
  }
};

template <typename K, typename V, typename D>
struct ArraySerializer<std::pair<K, V>, D>
{
public:
  using Type       = std::pair<V, K>;
  using DriverType = D;

  template <typename Constructor>
  static void Serialize(Constructor &array_constructor, Type const &input)
  {
    auto array = array_constructor(2);
    array.Append(input.first);
    array.Append(input.second);
  }

  template <typename ArrayDeserializer>
  static void Deserialize(ArrayDeserializer &array, Type &output)
  {
    if (array.size() != 2)
    {
      throw SerializableException(std::string("std::pair must have exactly 2 elements."));
    }

    array.GetNextValue(output.first);
    array.GetNextValue(output.second);
  }
};

}  // namespace serializers
}  // namespace fetch
