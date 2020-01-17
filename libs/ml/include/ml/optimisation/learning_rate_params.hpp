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
namespace optimisers {
/**
 * Training annealing config
 * @tparam T
 */
template <typename T>
struct LearningRateParam
{
  using DataType = T;
  enum class LearningRateDecay : uint8_t
  {
    EXPONENTIAL,
    LINEAR,
    NONE
  };
  LearningRateDecay mode                   = LearningRateDecay::NONE;
  DataType          starting_learning_rate = fetch::math::Type<DataType>("0.001");
  DataType          ending_learning_rate   = starting_learning_rate / DataType{10000};
  DataType          linear_decay_rate      = starting_learning_rate / DataType{10000};
  DataType          exponential_decay_rate = fetch::math::Type<DataType>("0.999");
};
}  // namespace optimisers
}  // namespace ml

}  // namespace fetch
