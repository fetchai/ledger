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

#include "ml/optimisation/types.hpp"

namespace fetch {
namespace ml {
namespace model {

template <typename DataType>
struct ModelConfig
{
  using SizeType = fetch::math::SizeType;

  bool     early_stopping = false;
  bool     test           = false;
  SizeType patience       = 10;
  DataType min_delta      = DataType{0};

  fetch::ml::optimisers::LearningRateParam<DataType> learning_rate_param;

  SizeType batch_size  = SizeType(32);
  SizeType subset_size = fetch::ml::optimisers::SIZE_NOT_SET;

  bool        print_stats         = false;
  bool        save_graph          = false;
  std::string graph_save_location = "/tmp/graph";

  ModelConfig()
  {
    learning_rate_param.mode =
        fetch::ml::optimisers::LearningRateParam<DataType>::LearningRateDecay::EXPONENTIAL;
    learning_rate_param.starting_learning_rate = fetch::math::Type<DataType>("0.001");
    learning_rate_param.exponential_decay_rate = fetch::math::Type<DataType>("0.99");
  };
};

}  // namespace model
}  // namespace ml

namespace serializers {
/**
 * serializer for Optimiser
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
