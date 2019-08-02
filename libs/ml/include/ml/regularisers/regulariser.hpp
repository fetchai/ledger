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
#include "math/tensor.hpp"

namespace fetch {
namespace ml {
namespace regularisers {

/*
 * Abstract Regulariser interface
 */
template <class T>
class Regulariser
{
public:
  using ArrayType = T;
  using DataType  = typename ArrayType::Type;

  Regulariser(RegularisationType rt)
    : reg_type(rt)
  {}

  virtual ~Regulariser()                                                            = default;
  virtual void ApplyRegularisation(ArrayType &weight, DataType regularisation_rate) = 0;

  RegularisationType reg_type;
};

}  // namespace regularisers
}  // namespace ml
namespace serializers {

/**
 * serializer for regularisers
 * @tparam TensorType
 */
template <typename TensorType, typename D>
struct MapSerializer<ml::regularisers::Regulariser<TensorType>, D>
{
  using Type       = ml::regularisers::Regulariser<TensorType>;
  using DriverType = D;

  static uint8_t const REG_TYPE = 1;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &sp)
  {
    auto map = map_constructor(1);
    map.Append(REG_TYPE, sp.reg_type);
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &sp)
  {
    map.ExpectKeyGetValue(REG_TYPE, sp.reg_type);
  }
};

}  // namespace serializers

}  // namespace fetch
