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

enum class RegularisationType : uint8_t
{
  NONE,
  L1,
  L2,
};
}

namespace serializers {

/**
 * serializer for OpType
 * @tparam TensorType
 */
template <typename D>
struct MapSerializer<ml::RegularisationType, D>
{
  using Type       = ml::RegularisationType;
  using DriverType = D;

  static uint8_t const REG_TYPE = 1;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &body)
  {
    auto map      = map_constructor(1);
    auto reg_type = static_cast<uint8_t>(body);
    map.Append(REG_TYPE, reg_type);
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &body)
  {
    uint8_t reg_type = 0;
    map.ExpectKeyGetValue(REG_TYPE, reg_type);
    body = static_cast<ml::RegularisationType>(reg_type);
  }
};

}  // namespace serializers

}  // namespace fetch
