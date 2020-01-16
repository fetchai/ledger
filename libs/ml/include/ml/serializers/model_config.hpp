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
template <typename DataType>
struct ModelConfig;
}  // namespace model
}  // namespace ml

namespace serializers {
/**
 * serializer for ModelConfig
 * @tparam TensorType
 */
template <typename DataType, typename D>
struct MapSerializer<fetch::ml::model::ModelConfig<DataType>, D>
{
  using Type       = fetch::ml::model::ModelConfig<DataType>;
  using DriverType = D;

  // public member variables
  static uint8_t const EARLY_STOPPING      = 1;
  static uint8_t const TEST                = 2;
  static uint8_t const PATIENCE            = 3;
  static uint8_t const MIN_DELTA           = 4;
  static uint8_t const LEARNING_RATE_PARAM = 6;
  static uint8_t const BATCH_SIZE          = 7;
  static uint8_t const SUSBET_SIZE         = 8;
  static uint8_t const PRINT_STATS         = 9;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &sp)
  {
    auto map = map_constructor(9);

    map.Append(EARLY_STOPPING, sp.early_stopping);
    map.Append(TEST, sp.test);
    map.Append(PATIENCE, sp.patience);
    map.Append(MIN_DELTA, sp.min_delta);
    map.Append(LEARNING_RATE_PARAM, sp.learning_rate_param);
    map.Append(BATCH_SIZE, sp.batch_size);
    map.Append(SUSBET_SIZE, sp.subset_size);
    map.Append(PRINT_STATS, sp.print_stats);
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &sp)
  {
    map.ExpectKeyGetValue(EARLY_STOPPING, sp.early_stopping);
    map.ExpectKeyGetValue(TEST, sp.test);
    map.ExpectKeyGetValue(PATIENCE, sp.patience);
    map.ExpectKeyGetValue(MIN_DELTA, sp.min_delta);
    map.ExpectKeyGetValue(LEARNING_RATE_PARAM, sp.learning_rate_param);
    map.ExpectKeyGetValue(BATCH_SIZE, sp.batch_size);
    map.ExpectKeyGetValue(SUSBET_SIZE, sp.subset_size);
    map.ExpectKeyGetValue(PRINT_STATS, sp.print_stats);
  }
};

}  // namespace serializers
}  // namespace fetch
