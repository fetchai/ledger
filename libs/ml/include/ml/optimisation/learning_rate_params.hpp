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

#include "math/base_types.hpp"
#include "math/statistics/mean.hpp"
#include "ml/dataloaders/dataloader.hpp"
#include "ml/graph.hpp"

#include <chrono>

namespace fetch {
namespace ml {
namespace optimisers {

}
/**
 * Training annealing config
 * @tparam T
 */
template <typename T>
struct LearningRateParam
{
  using DataType = T;
  enum class LearningRateDecay
  {
    EXPONENTIAL,
    LINEAR,
    NONE
  };
  LearningRateDecay mode                   = LearningRateDecay::NONE;
  DataType          starting_learning_rate = static_cast<DataType>(0.1);
  DataType          ending_learning_rate   = static_cast<DataType>(0.0001);
  DataType          linear_decay_rate      = static_cast<DataType>(0.0000000000001);
  DataType          exponential_decay_rate = static_cast<DataType>(0.999);


  template <typename S>
  friend void Serialize(S &serializer, LearningRateParam<T> const &lp)
  {
    serializer << lp.mode;
    serializer << lp.starting_learning_rate;
    serializer << lp.ending_learning_rate;
    serializer << lp.linear_decay_rate;
    serializer << lp.exponential_decay_rate;
  }

  template <typename S>
  friend void Deserialize(S &serializer, LearningRateParam<T> &lp)
  {

    serializer >> mode;
    serializer >> starting_learning_rate;
    serializer >> ending_learning_rate;
    serializer >> linear_decay_rate;
    serializer >> exponential_decay_rate;

//    LearningRateDecay mode;
//    DataType          starting_learning_rate;
//    DataType          ending_learning_rate;
//    DataType          linear_decay_rate;
//    DataType          exponential_decay_rate;
//
//    serializer >> mode;
//    serializer >> starting_learning_rate;
//    serializer >> ending_learning_rate;
//    serializer >> linear_decay_rate;
//    serializer >> exponential_decay_rate;
//
//    lp.mode = mode;
//    lp.starting_learning_rate = starting_learning_rate;
//    lp.ending_learning_rate = ending_learning_rate;
//    lp.linear_decay_rate = linear_decay_rate;
//    lp.exponential_decay_rate = exponential_decay_rate;
  }
};



}  // namespace optimisers
}  // namespace ml
}  // namespace fetch
