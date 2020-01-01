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

#include "math/base_types.hpp"
#include "math/standard_functions/pow.hpp"
#include "math/standard_functions/sqrt.hpp"
#include "ml/core/graph.hpp"
#include "ml/optimisation/optimiser.hpp"

#include <memory>
#include <string>
#include <vector>

namespace fetch {
namespace ml {
namespace optimisers {

/**
 * Adaptive Momentum optimiser
 * @tparam T TensorType
 * @tparam C CriterionType
 */
template <class T>
class AdamOptimiser : public Optimiser<T>
{
public:
  using TensorType = T;
  using DataType   = typename TensorType::Type;
  using SizeType   = fetch::math::SizeType;

  AdamOptimiser() = default;

  AdamOptimiser(std::shared_ptr<Graph<T>> graph, std::vector<std::string> const &input_node_names,
                std::string const &label_node_name, std::string const &output_node_name,
                DataType const &learning_rate = fetch::math::Type<DataType>("0.001"),
                DataType const &beta1         = fetch::math::Type<DataType>("0.9"),
                DataType const &beta2         = fetch::math::Type<DataType>("0.999"),
                DataType const &epsilon       = fetch::math::Type<DataType>("0.0001"));

  AdamOptimiser(std::shared_ptr<Graph<T>> graph, std::vector<std::string> const &input_node_names,
                std::string const &label_node_name, std::string const &output_node_name,
                fetch::ml::optimisers::LearningRateParam<DataType> const &learning_rate_param,
                DataType const &beta1   = fetch::math::Type<DataType>("0.9"),
                DataType const &beta2   = fetch::math::Type<DataType>("0.999"),
                DataType const &epsilon = fetch::math::Type<DataType>("0.0001"));

  ~AdamOptimiser() override = default;

  void ApplyGradients(SizeType batch_size) override;

  template <typename X, typename D>
  friend struct serializers::MapSerializer;

  OptimiserType OptimiserCode() override
  {
    return OptimiserType::ADAM;
  }

protected:
  std::vector<TensorType> cache_;
  std::vector<TensorType> momentum_;
  std::vector<TensorType> mt_;
  std::vector<TensorType> vt_;

  DataType beta1_;
  DataType beta2_;
  DataType beta1_t_;
  DataType beta2_t_;
  DataType epsilon_;

  void ResetCache();
  void Init();
};

template <class T>
void AdamOptimiser<T>::Init()
{
  for (auto &train : this->graph_trainables_)
  {
    this->cache_.emplace_back(TensorType(train->GetWeights().shape()));
    this->momentum_.emplace_back(TensorType(train->GetWeights().shape()));
    this->mt_.emplace_back(TensorType(train->GetWeights().shape()));
    this->vt_.emplace_back(TensorType(train->GetWeights().shape()));
  }
  ResetCache();
}

template <class T>
AdamOptimiser<T>::AdamOptimiser(std::shared_ptr<Graph<T>>       graph,
                                std::vector<std::string> const &input_node_names,
                                std::string const &             label_node_name,
                                std::string const &output_node_name, DataType const &learning_rate,
                                DataType const &beta1, DataType const &beta2,
                                DataType const &epsilon)
  : Optimiser<T>(graph, input_node_names, label_node_name, output_node_name, learning_rate)
  , beta1_(beta1)
  , beta2_(beta2)
  , beta1_t_(beta1)
  , beta2_t_(beta2)
  , epsilon_(epsilon)
{
  Init();
}

template <class T>
AdamOptimiser<T>::AdamOptimiser(
    std::shared_ptr<Graph<T>> graph, std::vector<std::string> const &input_node_names,
    std::string const &label_node_name, std::string const &output_node_name,
    fetch::ml::optimisers::LearningRateParam<DataType> const &learning_rate_param,
    DataType const &beta1, DataType const &beta2, DataType const &epsilon)
  : Optimiser<T>(graph, input_node_names, label_node_name, output_node_name, learning_rate_param)
  , beta1_(beta1)
  , beta2_(beta2)
  , beta1_t_(beta1)
  , beta2_t_(beta2)
  , epsilon_(epsilon)
{
  Init();
}

// private

template <class T>
void AdamOptimiser<T>::ApplyGradients(SizeType batch_size)
{
  // Do operation with gradient
  auto cached_weight_it = cache_.begin();
  auto momentum_it      = momentum_.begin();
  auto mt_it            = mt_.begin();
  auto vt_it            = vt_.begin();

  auto gradient_it  = this->gradients_.begin();
  auto trainable_it = this->graph_trainables_.begin();

  // beta1_t=beta1^t and beta2_t=beta2^t, where t is number of epochs + 1
  fetch::math::Pow(beta1_, static_cast<DataType>(this->epoch_ + 1), beta1_t_);
  fetch::math::Pow(beta2_, static_cast<DataType>(this->epoch_ + 1), beta2_t_);

  while (gradient_it != this->gradients_.end())
  {
    // Skip frozen trainables
    if (!(*trainable_it)->GetFrozenState())
    {

      // cache[i] = (beta1_t_ * cache[i]) + ((1.0 - beta1_t_) * (input_gradients[i]/batch_size));
      fetch::math::Multiply((*trainable_it)->GetGradientsReferences(),
                            (DataType{1} - beta1_t_) / static_cast<DataType>(batch_size),
                            *gradient_it);
      fetch::math::Multiply(*cached_weight_it, beta1_t_, *cached_weight_it);
      fetch::math::Add(*cached_weight_it, *gradient_it, *cached_weight_it);

      // mt   = cache[i] / (1.0 - beta1_t_);
      fetch::math::Divide(*cached_weight_it, (DataType{1} - beta1_t_), *mt_it);

      // momentum[i] = (beta2_t_ * momentum[i]) + ((1.0 - beta2_t_) *
      // ((input_gradients[i]/batch_size)^2));
      fetch::math::Divide((*trainable_it)->GetGradientsReferences(),
                          static_cast<DataType>(batch_size), *vt_it);
      fetch::math::Square(*vt_it, *vt_it);
      fetch::math::Multiply(*vt_it, (DataType{1} - beta2_t_), *vt_it);
      fetch::math::Multiply(*momentum_it, beta2_t_, *momentum_it);
      fetch::math::Add(*momentum_it, *vt_it, *momentum_it);

      // vt   = momentum[i] / (1.0 - beta2_t_);
      fetch::math::Divide(*momentum_it, (DataType{1} - beta2_t_), *vt_it);

      // output_gradients[i] = -this->learning_rate_ * mt / (sqrt(vt) + epsilon_);
      fetch::math::Sqrt(*vt_it, *gradient_it);
      fetch::math::Add(*gradient_it, epsilon_, *gradient_it);
      fetch::math::Divide(*mt_it, *gradient_it, *gradient_it);
      fetch::math::Multiply(*gradient_it, -this->learning_rate_, *gradient_it);

      // we need to explicitly reset the gradients for this shared op to avoid double counting
      // in the case of shared ops
      (*trainable_it)->ResetGradients();
    }

    ++cached_weight_it;
    ++momentum_it;
    ++mt_it;
    ++vt_it;

    ++gradient_it;
    ++trainable_it;
  }

  // calling apply gradients on the graph ensures that the node caches are reset properly
  this->graph_->ApplyGradients(this->gradients_);
}

template <class T>
void AdamOptimiser<T>::ResetCache()
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

namespace serializers {
/**
 * serializer for SGDOptimiser
 * @tparam TensorType
 */
template <typename TensorType, typename D>
struct MapSerializer<ml::optimisers::AdamOptimiser<TensorType>, D>
{
  using Type                          = ml::optimisers::AdamOptimiser<TensorType>;
  using DriverType                    = D;
  static uint8_t const BASE_OPTIMISER = 1;
  static uint8_t const CACHE          = 2;
  static uint8_t const MOMENTUM       = 3;
  static uint8_t const MT             = 4;
  static uint8_t const VT             = 5;
  static uint8_t const BETA1          = 6;
  static uint8_t const BETA2          = 7;
  static uint8_t const BETA1_T        = 8;
  static uint8_t const BETA2_T        = 9;
  static uint8_t const EPSILON        = 10;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &sp)
  {
    auto map = map_constructor(10);

    // serialize the optimiser parent class
    auto base_pointer = static_cast<ml::optimisers::Optimiser<TensorType> const *>(&sp);
    map.Append(BASE_OPTIMISER, *base_pointer);

    map.Append(CACHE, sp.cache_);
    map.Append(MOMENTUM, sp.momentum_);
    map.Append(MT, sp.mt_);
    map.Append(VT, sp.vt_);
    map.Append(BETA1, sp.beta1_);
    map.Append(BETA2, sp.beta2_);
    map.Append(BETA1_T, sp.beta1_t_);
    map.Append(BETA2_T, sp.beta2_t_);
    map.Append(EPSILON, sp.epsilon_);
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &sp)
  {
    auto base_pointer = static_cast<ml::optimisers::Optimiser<TensorType> *>(&sp);
    map.ExpectKeyGetValue(BASE_OPTIMISER, *base_pointer);

    map.ExpectKeyGetValue(CACHE, sp.cache_);
    map.ExpectKeyGetValue(MOMENTUM, sp.momentum_);
    map.ExpectKeyGetValue(MT, sp.mt_);
    map.ExpectKeyGetValue(VT, sp.vt_);
    map.ExpectKeyGetValue(BETA1, sp.beta1_);
    map.ExpectKeyGetValue(BETA2, sp.beta2_);
    map.ExpectKeyGetValue(BETA1_T, sp.beta1_t_);
    map.ExpectKeyGetValue(BETA2_T, sp.beta2_t_);
    map.ExpectKeyGetValue(EPSILON, sp.epsilon_);
  }
};
}  // namespace serializers

}  // namespace fetch
