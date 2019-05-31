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
#include "ml/optimisation/optimiser.hpp"

namespace fetch {
namespace ml {
namespace optimisers {

/**
 * Adaptive Momentum optimiser
 * @tparam T ArrayType
 * @tparam C CriterionType
 */
template <class T, class C>
class AdamOptimiser : public Optimiser<T, C>
{
public:
  using ArrayType = T;
  using DataType  = typename ArrayType::Type;
  using SizeType  = typename ArrayType::SizeType;

  AdamOptimiser(std::shared_ptr<Graph<T>>

                                   graph,
                std::string const &input_node_name, std::string const &output_node_name,
                DataType const &learning_rate = DataType{0.001f},
                DataType const &beta1 = DataType{0.9f}, DataType const &beta2 = DataType{0.999f},
                DataType const &epsilon = DataType{1e-4f});

private:
  std::vector<ArrayType> cache_;
  std::vector<ArrayType> momentum_;
  std::vector<ArrayType> mt_;
  std::vector<ArrayType> vt_;

  DataType beta1_;
  DataType beta2_;
  DataType beta1_t_;
  DataType beta2_t_;
  DataType epsilon_;
  DataType one_{1};

  void ApplyGradients() override;
  void ResetCache();
};

template <class T, class C>
AdamOptimiser<T, C>::AdamOptimiser(std::shared_ptr<Graph<T>>

                                                      graph,
                                   std::string const &input_node_name,
                                   std::string const &output_node_name,
                                   DataType const &learning_rate, DataType const &beta1,
                                   DataType const &beta2, DataType const &epsilon)
  : Optimiser<T, C>(graph, input_node_name, output_node_name, learning_rate)
  , beta1_(beta1)
  , beta2_(beta2)
  , beta1_t_(beta1)
  , beta2_t_(beta2)
  , epsilon_(epsilon)
{
  for (auto &train : this->graph_trainables_)
  {
    this->cache_.emplace_back(ArrayType(train->GetWeights().shape()));
    this->momentum_.emplace_back(ArrayType(train->GetWeights().shape()));
    this->mt_.emplace_back(ArrayType(train->GetWeights().shape()));
    this->vt_.emplace_back(ArrayType(train->GetWeights().shape()));
  }
  ResetCache();
}

// private

template <class T, class C>
void AdamOptimiser<T, C>::ApplyGradients()
{
  // Do operation with gradient
  auto cached_weight_it = cache_.begin();
  auto momentum_it      = momentum_.begin();
  auto mt_it            = mt_.begin();
  auto vt_it            = vt_.begin();

  auto gradient_it  = this->gradients_.begin();
  auto trainable_it = this->graph_trainables_.begin();

  while (gradient_it != this->gradients_.end())
  {
    // cache[i] = (beta1_t_ * cache[i]) + ((one_ - beta1_t_) * (gradients[i]));
    fetch::math::Multiply((*trainable_it)->Gradients(), (one_ - beta1_t_), *gradient_it);
    fetch::math::Multiply(*cached_weight_it, beta1_t_, *cached_weight_it);
    fetch::math::Add(*cached_weight_it, *gradient_it, *cached_weight_it);

    // mt   = cache[i] / (one_ - beta1_t_);
    fetch::math::Divide(*cached_weight_it, (one_ - beta1_t_), *mt_it);

    // momentum[i] = (beta2_t_ * momentum[i]) + ((one_ - beta2_t_) * (gradients[i]^2));
    fetch::math::Multiply((*trainable_it)->Gradients(), (*trainable_it)->Gradients(), *vt_it);
    fetch::math::Multiply(*vt_it, (one_ - beta2_t_), *vt_it);
    fetch::math::Multiply(*momentum_it, beta2_t_, *momentum_it);
    fetch::math::Add(*momentum_it, *vt_it, *momentum_it);

    // vt   = momentum[i] / (one_ - beta2_t_);
    fetch::math::Divide(*momentum_it, (one_ - beta2_t_), *vt_it);

    // gradients[i] = -this->learning_rate_ * mt / (sqrt(vt) + epsilon_);
    fetch::math::Sqrt(*vt_it, *gradient_it);
    fetch::math::Add(*gradient_it, epsilon_, *gradient_it);
    fetch::math::Divide(*mt_it, *gradient_it, *gradient_it);
    fetch::math::Multiply(*gradient_it, -this->learning_rate_, *gradient_it);

    // Apply gradient weights[i]+=grad[i]
    (*trainable_it)->ApplyGradient(*gradient_it);

    ++cached_weight_it;
    ++momentum_it;
    ++mt_it;
    ++vt_it;

    ++gradient_it;
    ++trainable_it;
  }

  // beta1_t=beta1^t and beta2_t=beta2^t, where t is number of epochs + 1
  beta1_t_ *= beta1_;
  beta2_t_ *= beta2_;
}

template <class T, class C>
void AdamOptimiser<T, C>::ResetCache()
{
  for (auto &val : this->cache_)
  {
    val.Fill(DataType{0});
  }
  for (auto &moment : this->momentum_)
  {
    moment.Fill(DataType{0});
  }
  beta1_t_ = beta1_;
  beta2_t_ = beta2_;
}

}  // namespace optimisers
}  // namespace ml
}  // namespace fetch