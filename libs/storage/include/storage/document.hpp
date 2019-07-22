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

namespace fetch {
namespace storage {

struct Document
{
  explicit operator byte_array::ConstByteArray()
  {
    return document;
  }

  byte_array::ByteArray document;
  bool                  was_created = false;
  bool                  failed      = false;
};

}  // namespace storage

namespace serializers {

template <typename D>
struct MapSerializer<storage::Document, D>
{
public:
  using Type       = storage::Document;
  using DriverType = D;

  static uint8_t const DOCUMENT    = 1;
  static uint8_t const WAS_CREATED = 2;
  static uint8_t const FAILED      = 3;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &data)
  {
    auto map = map_constructor(3);
    map.Append(DOCUMENT, data.document);
    map.Append(WAS_CREATED, data.was_created);
    map.Append(FAILED, data.failed);
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &data)
  {
    map.ExpectKeyGetValue(DOCUMENT, data.document);
    map.ExpectKeyGetValue(WAS_CREATED, data.was_created);
    map.ExpectKeyGetValue(FAILED, data.failed);
  }
};

}  // namespace serializers
}  // namespace fetch
