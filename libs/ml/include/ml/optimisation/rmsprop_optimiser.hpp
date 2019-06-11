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

#include "math/standard_functions/sqrt.hpp"
#include "ml/graph.hpp"
#include "ml/ops/loss_functions/criterion.hpp"
#include "optimiser.hpp"

namespace fetch {
namespace ml {
namespace optimisers {

/**
 * Root Mean Square Propagation optimiser explained in paper:
 * https://www.cs.toronto.edu/~tijmen/csc321/slides/lecture_slides_lec6.pdf
 * @tparam T ArrayType
 * @tparam C CriterionType
 */
template <class T, class C>
class RMSPropOptimiser : public Optimiser<T, C>
{
public:
  using ArrayType = T;
  using DataType  = typename ArrayType::Type;
  using SizeType  = typename ArrayType::SizeType;

  RMSPropOptimiser(std::shared_ptr<Graph<T>>

                                                   graph,
                   std::vector<std::string> const &input_node_names,
                   std::string const &             output_node_name,
                   DataType const &                learning_rate = DataType{0.001f},
                   DataType const &                decay_rate    = DataType{0.9f},
                   DataType const &                epsilon       = DataType{1e-8f});

private:
  std::vector<ArrayType> cache_;
  DataType               decay_rate_;
  DataType               one_{1};
  DataType               epsilon_;

  void ApplyGradients(SizeType batch_size) override;
  void ResetCache();
};

template <class T, class C>
RMSPropOptimiser<T, C>::RMSPropOptimiser(std::shared_ptr<Graph<T>>

                                                                         graph,
                                         std::vector<std::string> const &input_node_names,
                                         std::string const &             output_node_name,
                                         DataType const &learning_rate, DataType const &decay_rate,
                                         DataType const &epsilon)
  : Optimiser<T, C>(graph, input_node_names, output_node_name, learning_rate)
  ,

  decay_rate_(decay_rate)
  , epsilon_(epsilon)
{
  auto weights = this->graph_->get_weights();
  for (auto &train : this->graph_trainables_)
  {
    this->cache_.emplace_back(ArrayType(train->get_weights().shape()));
  }

  ResetCache();
}

// private

template <class T, class C>
void RMSPropOptimiser<T, C>::ApplyGradients(SizeType batch_size)
{
  // Do operation with gradient
  auto cached_weight_it = cache_.begin();
  auto gradient_it      = this->gradients_.begin();
  auto trainable_it     = this->graph_trainables_.begin();

  while (gradient_it != this->gradients_.end())
  {
    // cache[i] = decay_rate * cache[i] + (1 - decay_rate) * ((input_grad[i]/batch_size)^2)
    fetch::math::Divide((*trainable_it)->get_gradients(), static_cast<DataType>(batch_size),
                        *gradient_it);
    fetch::math::Square(*gradient_it, *gradient_it);

    fetch::math::Multiply(*gradient_it, (one_ - decay_rate_), *gradient_it);
    fetch::math::Multiply(*cached_weight_it, decay_rate_, *cached_weight_it);
    fetch::math::Add(*cached_weight_it, *gradient_it, *cached_weight_it);

    // epsilon is added to prevent division by 0
    // output_grad[i] = learning_rate * (input_grad[i]/batch_size) / (sqrt(cache[i]) + epsilon)
    fetch::math::Sqrt(*cached_weight_it, *gradient_it);
    fetch::math::Add(*gradient_it, epsilon_, *gradient_it);
    fetch::math::Divide((*trainable_it)->get_gradients(), *gradient_it, *gradient_it);
    fetch::math::Multiply(
        *gradient_it, (-this->learning_rate_) / (static_cast<DataType>(batch_size)), *gradient_it);

    // Apply gradient weights[i]+=output_grad[i]
    (*trainable_it)->ApplyGradient(*gradient_it);

    ++cached_weight_it;
    ++gradient_it;
    ++trainable_it;
  }
}

template <class T, class C>
void RMSPropOptimiser<T, C>::ResetCache()
{
  for (auto &val : this->cache_)
  {
    val.Fill(DataType{0});
  }
}

}  // namespace optimisers
}  // namespace ml
}  // namespace fetch
