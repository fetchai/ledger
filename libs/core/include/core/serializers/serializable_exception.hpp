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
#include "core/serializers/group_definitions.hpp"
#include "core/serializers/exception.hpp"

#include <type_traits>

namespace fetch {
namespace serializers {

template <typename D>
struct MapSerializer<SerializableException, D>
{
public:
  using Type       = SerializableException;
  using DriverType = D;

  static const uint8_t KEY_CODE        = 1;
  static const uint8_t KEY_EXPLANATION = 2;

  template <typename T>
  static inline void Serialize(T &map_constructor, Type const &s)
  {
    error::error_type code = s.error_code();

    auto map = map_constructor(2);
    map.Append(KEY_CODE, code);
    map.Append(KEY_EXPLANATION, s.explanation());
    /*
     serializer.Allocate(sizeof(error::error_type) + sizeof(uint64_t) + s.explanation().size());

     serializer.WriteBytes(reinterpret_cast<uint8_t const *>(&code), sizeof(error::error_type));
     serializer.WriteBytes(reinterpret_cast<uint8_t const *>(&size), sizeof(uint64_t));
     serializer.WriteBytes(reinterpret_cast<uint8_t const *>(s.explanation().c_str()), size);
     */
  }

  template <typename T>
  static inline void Deserialize(T &map, Type &s)
  {
    error::error_type     code;
    byte_array::ByteArray buffer;
    uint8_t               key;

    map.GetNextKeyPair(key, code);  // TODO: Test keys
    map.GetNextKeyPair(key, buffer);
    /*
        serializer.ReadBytes(reinterpret_cast<uint8_t *>(&code), sizeof(error::error_type));
        serializer.ReadBytes(reinterpret_cast<uint8_t *>(&size), sizeof(uint64_t));

        byte_array::ByteArray buffer;
        buffer.Resize(size);
        serializer.ReadBytes(buffer.pointer(), size);
    */
    s = Type(code, buffer);
  }
};

}  // namespace serializers
}  // namespace fetch
