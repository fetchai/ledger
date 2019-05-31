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

#include "ml/graph.hpp"
#include "ml/ops/loss_functions/criterion.hpp"
#include "optimiser.hpp"

namespace fetch {
namespace ml {
namespace optimisers {

/**
 * Stochastic Gradient Descent optimiser
 * @tparam T ArrayType
 * @tparam C CriterionType
 */
template <class T, class C>
class SGDOptimiser : public Optimiser<T, C>
{
public:
  using ArrayType = T;
  using DataType  = typename ArrayType::Type;
  using SizeType  = typename ArrayType::SizeType;

  SGDOptimiser(std::shared_ptr<Graph<T>>

                                  graph,
               std::string const &input_node_name, std::string const &output_node_name,
               DataType const &learning_rate)
    : Optimiser<T, C>(graph, input_node_name, output_node_name, learning_rate)
  {}

private:
  void ApplyGradients()
  {
    auto trainable_it = this->graph_trainables_.begin();
    auto gradient_it  = this->gradients_.begin();

    while (gradient_it != this->gradients_.end())
    {
      // grad[i]=grad[i] * -learning_rate
      fetch::math::Multiply((*trainable_it)->Gradients(), -this->learning_rate_, *gradient_it);

      // Apply gradient weights[i]+=grad[i]
      (*trainable_it)->ApplyGradient(*gradient_it);

      ++trainable_it;
      ++gradient_it;
    }
  }
};

}  // namespace optimisers
}  // namespace ml
}  // namespace fetch