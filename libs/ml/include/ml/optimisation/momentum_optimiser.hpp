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
#include "ml/optimisation/optimiser.hpp"

namespace fetch {
namespace ml {
namespace optimisers {

/**
 * Adaptive Momentum optimiser
 * i.e.  Stochastic gradient descent with momentum
 * @tparam T ArrayType
 * @tparam C CriterionType
 */
template <class T, class C>
class MomentumOptimiser : public Optimiser<T, C>
{
public:
  using ArrayType = T;
  using DataType  = typename ArrayType::Type;
  using SizeType  = typename ArrayType::SizeType;

  MomentumOptimiser(std::shared_ptr<Graph<T>>

                                       graph,
                    std::string const &input_node_name, std::string const &output_node_name,
                    DataType const &learning_rate   = DataType{0.001f},
                    DataType const &momentum_update = DataType{0.9f});

private:
  std::vector<ArrayType> momentum_;
  DataType               momentum_update_;
  DataType               negative_one_{-1};
  DataType               zero_{0};

  void ApplyGradients(SizeType batch_size) override;
  void ResetMomentum();
};

template <class T, class C>
MomentumOptimiser<T, C>::MomentumOptimiser(std::shared_ptr<Graph<T>>

                                                              graph,
                                           std::string const &input_node_name,
                                           std::string const &output_node_name,
                                           DataType const &   learning_rate,
                                           DataType const &   momentum_update)
  : Optimiser<T, C>(graph, input_node_name, output_node_name, learning_rate)
  , momentum_update_(momentum_update)
{
  for (auto &train : this->graph_trainables_)
  {
    this->momentum_.emplace_back(ArrayType(train->get_weights().shape()));
  }
  ResetMomentum();
}

// private

template <class T, class C>
void MomentumOptimiser<T, C>::ApplyGradients(SizeType batch_size)
{
  auto trainable_it = this->graph_trainables_.begin();
  auto gradient_it  = this->gradients_.begin();
  auto mit          = momentum_.begin();

  while (gradient_it != this->gradients_.end())
  {
    // momentum[i] = momentum_update * momentum[i] + learning_rate * (input_grad[i]/batch_size)
    fetch::math::Multiply(*mit, momentum_update_, *mit);
    fetch::math::Multiply((*trainable_it)->get_gradients(),
                          (this->learning_rate_) / (static_cast<DataType>(batch_size)),
                          *gradient_it);
    fetch::math::Add(*mit, *gradient_it, *mit);

    // output_grad[i]=-momentum[i]
    fetch::math::Multiply(*mit, negative_one_, *gradient_it);

    // Apply gradient weights[i]+=output_grad[i]
    (*trainable_it)->ApplyGradient(*gradient_it);

    ++trainable_it;
    ++gradient_it;
    ++mit;
  }
}

template <class T, class C>
void MomentumOptimiser<T, C>::ResetMomentum()
{
  for (auto &moment : this->momentum_)
  {
    moment.Fill(zero_);
  }
}

}  // namespace optimisers
}  // namespace ml
}  // namespace fetch
