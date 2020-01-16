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
#include "ml/serializers/model_config.hpp"

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
  }
};
}  // namespace model
}  // namespace ml
}  // namespace fetch
