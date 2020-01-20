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

namespace fetch {

namespace ml {
namespace model {
template <typename TensorType>
class Sequential;
template <typename DataType>
class Model;
}  // namespace model
}  // namespace ml

namespace serializers {
/**
 * serializer for Sequential model
 * @tparam TensorType
 */
template <typename TensorType, typename D>
struct MapSerializer<ml::model::Sequential<TensorType>, D>
{
  using Type                          = ml::model::Sequential<TensorType>;
  using DriverType                    = D;
  static uint8_t const BASE_MODEL     = 1;
  static uint8_t const LAYER_COUNT    = 2;
  static uint8_t const PREV_LAYER_STR = 3;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &sp)
  {
    auto map = map_constructor(3);

    // serialize the optimiser parent class
    auto base_pointer = static_cast<ml::model::Model<TensorType> const *>(&sp);
    map.Append(BASE_MODEL, *base_pointer);
    map.Append(LAYER_COUNT, sp.layer_count_);
    map.Append(PREV_LAYER_STR, sp.prev_layer_);
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &sp)
  {
    auto base_pointer = static_cast<ml::model::Model<TensorType> *>(&sp);
    map.ExpectKeyGetValue(BASE_MODEL, *base_pointer);
    map.ExpectKeyGetValue(LAYER_COUNT, sp.layer_count_);
    map.ExpectKeyGetValue(PREV_LAYER_STR, sp.prev_layer_);
  }
};
}  // namespace serializers
}  // namespace fetch
