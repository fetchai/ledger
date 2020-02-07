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

#include "ml/regularisers/regulariser.hpp"

namespace fetch {
namespace ml {
namespace regularisers {

/*
 * L2 regularisation
 */
template <class T>
class L2Regulariser : public Regulariser<T>
{
public:
  using TensorType = T;
  using DataType   = typename TensorType::Type;

  L2Regulariser();
  ~L2Regulariser() override = default;

  void ApplyRegularisation(TensorType &weight, DataType regularisation_rate) override;
};

}  // namespace regularisers
}  // namespace ml

namespace serializers {

/**
 * serializer for regularisers
 * @tparam TensorType
 */
template <typename TensorType, typename D>
struct MapSerializer<ml::regularisers::L2Regulariser<TensorType>, D>
{
  using Type       = ml::regularisers::L2Regulariser<TensorType>;
  using DriverType = D;

  static uint8_t const REG_TYPE = 1;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &sp)
  {
    auto map = map_constructor(1);
    map.Append(REG_TYPE, static_cast<uint8_t>(sp.GetRegType()));
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &sp)
  {
    FETCH_UNUSED(map);
    FETCH_UNUSED(sp);
  }
};

}  // namespace serializers

}  // namespace fetch
