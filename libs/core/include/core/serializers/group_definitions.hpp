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

namespace fetch {
namespace serializers {

struct TypeCodes
{
  enum
  {
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

    BINARY_CODE_FIXED = 0xc4,
    BINARY_CODE16     = 0xc5,
    BINARY_CODE32     = 0xc6,

    ARRAY_CODE_FIXED = 0x90,
    ARRAY_CODE16     = 0xdc,
    ARRAY_CODE32     = 0xdd,

    MAP_CODE_FIXED = 0x80,
    MAP_CODE16     = 0xde,
    MAP_CODE32     = 0xdf,

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
struct ExtensionSerializer;

}  // namespace serializers
}  // namespace fetch
