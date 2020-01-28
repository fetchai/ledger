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

#include "ml/serializers/ops/ops.hpp"
#include "ml/utilities/graph_builder.hpp"

namespace fetch {
namespace serializers {

/**
 * serializer for abs saveable params
 * @tparam TensorType
 */
template <typename TensorType, typename D>
struct MapSerializer<ml::ops::Abs<TensorType>, D>
{
  using Type       = ml::ops::Abs<TensorType>;
  using DriverType = D;

  static uint8_t const BASE_OPS = 1;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &sp)
  {
    auto map = map_constructor(1);

    // serialize parent class first
    auto ops_pointer = static_cast<ml::ops::Ops<TensorType> const *>(&sp);
    map.Append(BASE_OPS, *(ops_pointer));
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &sp)
  {
    auto ops_pointer = static_cast<ml::ops::Ops<TensorType> *>(&sp);
    map.ExpectKeyGetValue(BASE_OPS, (*ops_pointer));
  }
};

}  // namespace serializers
}  // namespace fetch
