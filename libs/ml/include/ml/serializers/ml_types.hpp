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

#include "ml/ops/weights.hpp"

namespace fetch {
namespace serializers {

template <typename V, typename D>
struct MapSerializer<ml::StateDict<V>, D>
{
public:
  using Type       = ml::StateDict<V>;
  using DriverType = D;

  constexpr static uint8_t WEIGHTS = 1;
  constexpr static uint8_t DICT    = 2;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &sd)
  {
    uint64_t n = 0;
    if (sd.weights_)
    {
      ++n;
    }
    if (!sd.dict_.empty())
    {
      ++n;
    }
    auto map = map_constructor(n);
    if (sd.weights_)
    {
      map.Append(WEIGHTS, *sd.weights_);
    }
    if (!sd.dict_.empty())
    {
      map.Append(DICT, sd.dict_);
    }
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &output)
  {
    for (uint64_t i = 0; i < map.size(); ++i)
    {
      uint8_t key;
      map.GetKey(key);
      switch (key)
      {
      case WEIGHTS:
        output.weights_ = std::make_shared<V>();
        map.GetValue(*output.weights_);
        break;
      case DICT:
        map.GetValue(output.dict_);
        break;
      default:
        throw std::runtime_error("unsupported key in statemap deserialization");
      }
    }
  }
};

}  // namespace serializers
}  // namespace fetch
