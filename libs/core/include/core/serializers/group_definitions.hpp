#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018-2020 Fetch.AI Limited
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

#include <cstdint>

namespace fetch {
namespace serializers {

enum class SerializerTypes
{
  BOOLEAN,
  INTEGER,
  UNSIGNED_INTEGER,
  FLOATING_POINT,
  BINARY,
  ARRAY,
  MAP,
  STRING,
  EXTENSION,
  NULL_VALUE,
  UNKNOWN
};

struct TypeCodes
{
  enum
  {
    NIL = 0xc0,

    BOOL_TRUE  = 0xc3,
    BOOL_FALSE = 0xc2,
    INT8       = 0xd0,
    INT16      = 0xd1,
    INT32      = 0xd2,
    INT64      = 0xd3,

    UINT8  = 0xcc,
    UINT16 = 0xcd,
    UINT32 = 0xce,
    UINT64 = 0xcf,

    FLOAT  = 0xca,
    DOUBLE = 0xcb,

    BINARY_CODE8  = 0xc4,
    BINARY_CODE16 = 0xc5,
    BINARY_CODE32 = 0xc6,

    EXTENSION_CODE8   = 0xc7,  // TODO(tfr): Make EXTENSION implementation
    EXTENSION_CODE16  = 0xc8,
    EXTENSION_CODE32  = 0xc9,
    EXTENSION_FIXED1  = 0xd4,
    EXTENSION_FIXED2  = 0xd5,
    EXTENSION_FIXED4  = 0xd6,
    EXTENSION_FIXED8  = 0xd7,
    EXTENSION_FIXED16 = 0xd8,

    ARRAY_CODE_FIXED = 0x90,
    ARRAY_CODE16     = 0xdc,
    ARRAY_CODE32     = 0xdd,

    MAP_CODE_FIXED = 0x80,
    MAP_CODE16     = 0xde,
    MAP_CODE32     = 0xdf,

    PAIR_CODE_FIXED = 0x70,
    PAIR_CODE16     = 0xb0,
    PAIR_CODE32     = 0xb1,

    FIXED_MASK1     = 0xF0,
    FIXED_MASK2     = 0xE0,
    FIXED_VAL_MASK  = 0x0F,
    FIXED_VAL_MASK2 = 0x1F,

    STRING_CODE_FIXED = 0xa0,
    STRING_CODE8      = 0xd9,
    STRING_CODE16     = 0xda,
    STRING_CODE32     = 0xdb
  };
};

inline SerializerTypes DetermineType(uint8_t b)
{
  switch (b)
  {
  case TypeCodes::NIL:
    return SerializerTypes::NULL_VALUE;
  case TypeCodes::BOOL_TRUE:
  case TypeCodes::BOOL_FALSE:
    return SerializerTypes::BOOLEAN;
  case TypeCodes::INT8:
  case TypeCodes::INT16:
  case TypeCodes::INT32:
  case TypeCodes::INT64:
    return SerializerTypes::INTEGER;
  case TypeCodes::UINT8:
  case TypeCodes::UINT16:
  case TypeCodes::UINT32:
  case TypeCodes::UINT64:
    return SerializerTypes::UNSIGNED_INTEGER;
  case TypeCodes::FLOAT:
  case TypeCodes::DOUBLE:
    return SerializerTypes::FLOATING_POINT;
  case TypeCodes::BINARY_CODE8:
  case TypeCodes::BINARY_CODE16:
  case TypeCodes::BINARY_CODE32:
    return SerializerTypes::BINARY;
  case TypeCodes::EXTENSION_CODE8:
  case TypeCodes::EXTENSION_CODE16:
  case TypeCodes::EXTENSION_CODE32:
  case TypeCodes::EXTENSION_FIXED1:
  case TypeCodes::EXTENSION_FIXED2:
  case TypeCodes::EXTENSION_FIXED4:
  case TypeCodes::EXTENSION_FIXED8:
  case TypeCodes::EXTENSION_FIXED16:
    return SerializerTypes::EXTENSION;
  case TypeCodes::ARRAY_CODE16:
  case TypeCodes::ARRAY_CODE32:
    return SerializerTypes::ARRAY;
  case TypeCodes::MAP_CODE16:
  case TypeCodes::MAP_CODE32:
    return SerializerTypes::MAP;
  case TypeCodes::STRING_CODE8:
  case TypeCodes::STRING_CODE16:
  case TypeCodes::STRING_CODE32:
    return SerializerTypes::STRING;
  }

  switch (b & TypeCodes::FIXED_MASK1)
  {
  case TypeCodes::ARRAY_CODE_FIXED:
    return SerializerTypes::ARRAY;
  case TypeCodes::MAP_CODE_FIXED:
    return SerializerTypes::MAP;
  }

  switch (b & TypeCodes::FIXED_MASK2)
  {
  case TypeCodes::STRING_CODE_FIXED:
    return SerializerTypes::STRING;
  }

  if (b < 128)
  {
    return SerializerTypes::UNSIGNED_INTEGER;
  }

  union
  {
    uint8_t code;
    int8_t  value;
  } conversion{};
  conversion.code = b;

  if (-0x20 <= conversion.value)
  {
    return SerializerTypes::INTEGER;
  }

  return SerializerTypes::UNKNOWN;
}

template <typename T, typename D>
struct IgnoredSerializer;

template <typename T, typename D>
struct ForwardSerializer;

template <typename T, typename D>
struct IntegerSerializer;

template <typename T, typename D>
struct BooleanSerializer;

template <typename T, typename D>
struct FloatSerializer;

template <typename T, typename D>
struct StringSerializer;

template <typename T, typename D>
struct BinarySerializer;

template <typename T, typename D>
struct ArraySerializer;

template <typename T, typename D>
struct MapSerializer;

template <typename T, typename D>
struct PairSerializer;

template <typename T, typename D>
struct ExtensionSerializer;

}  // namespace serializers
}  // namespace fetch
