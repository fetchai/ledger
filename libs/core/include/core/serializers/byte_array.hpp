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

#include "core/assert.hpp"
#include "core/byte_array/const_byte_array.hpp"

#include <type_traits>

namespace fetch {
namespace serializers {

template <typename T>
inline void Serialize(T &serializer, byte_array::ConstByteArray const &s)
{
  serializer.Allocate(sizeof(uint64_t) + s.size());
  uint64_t size = s.size();

  serializer.WriteBytes(reinterpret_cast<uint8_t const *>(&size), sizeof(uint64_t));
  serializer.WriteBytes(reinterpret_cast<uint8_t const *>(s.pointer()), s.size());
}

template <typename T>
inline void Deserialize(T &serializer, byte_array::ConstByteArray &s)
{
  uint64_t size = 0;

  detailed_assert(int64_t(sizeof(uint64_t)) <= serializer.bytes_left());
  serializer.ReadBytes(reinterpret_cast<uint8_t *>(&size), sizeof(uint64_t));
  detailed_assert(int64_t(size) <= serializer.bytes_left());

  serializer.ReadByteArray(s, size);
}

}  // namespace serializers
}  // namespace fetch
