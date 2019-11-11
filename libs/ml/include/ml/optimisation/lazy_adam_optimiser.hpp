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
class LazyAdamOptimiser : public Optimiser<T>
{
public:
  using TensorType = T;
  using DataType   = typename TensorType::Type;
  using SizeType   = fetch::math::SizeType;
  using SizeSet    = std::unordered_set<SizeType>;

  LazyAdamOptimiser() = default;

  LazyAdamOptimiser(std::shared_ptr<Graph<T>>       graph,
                    std::vector<std::string> const &input_node_names,
                    std::string const &label_node_name, std::string const &output_node_name,
                    DataType const &learning_rate = static_cast<DataType>(0.001f),
                    DataType const &beta1         = static_cast<DataType>(0.9f),
                    DataType const &beta2         = static_cast<DataType>(0.999f),
                    DataType const &epsilon       = static_cast<DataType>(1e-4f));

  LazyAdamOptimiser(std::shared_ptr<Graph<T>>       graph,
                    std::vector<std::string> const &input_node_names,
                    std::string const &label_node_name, std::string const &output_node_name,
                    fetch::ml::optimisers::LearningRateParam<DataType> const &learning_rate_param,
                    DataType const &beta1   = static_cast<DataType>(0.9f),
                    DataType const &beta2   = static_cast<DataType>(0.999f),
                    DataType const &epsilon = static_cast<DataType>(1e-4f));

  ~LazyAdamOptimiser() override = default;

  void ApplyGradients(SizeType batch_size) override;

  template <typename X, typename D>
  friend struct serializers::MapSerializer;

  OptimiserType OptimiserCode() override
  {
    return OptimiserType::ADAM;
  }

private:
  std::vector<TensorType> cache_;
  std::vector<TensorType> momentum_;
  std::vector<TensorType> mt_;
  std::vector<TensorType> vt_;

  DataType beta1_;
  DataType beta2_;
  DataType beta1_t_;
  DataType beta2_t_;
  DataType epsilon_;

  // ApplyGradientSparse if number_of_rows_to_update*sparsity_threshold_ <= total_rows
  SizeType sparsity_threshold_ = 4;

  void ResetCache();
  void Init();
  void ApplyLogic(SizeType batch_size, TensorType &gradient_tensor, TensorType &momentum_tensor,
                  TensorType &mt_tensor, TensorType &v_tensor, TensorType &cache_tensor,
                  TensorType const &refs_tensor);
};

template <class T>
void LazyAdamOptimiser<T>::Init()
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
LazyAdamOptimiser<T>::LazyAdamOptimiser(std::shared_ptr<Graph<T>>       graph,
                                        std::vector<std::string> const &input_node_names,
                                        std::string const &             label_node_name,
                                        std::string const &             output_node_name,
                                        DataType const &learning_rate, DataType const &beta1,
                                        DataType const &beta2, DataType const &epsilon)
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
LazyAdamOptimiser<T>::LazyAdamOptimiser(
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
void LazyAdamOptimiser<T>::ApplyLogic(SizeType batch_size, TensorType &gradient_tensor,
                                      TensorType &momentum_tensor, TensorType &mt_tensor,
                                      TensorType &v_tensor, TensorType &cache_tensor,
                                      TensorType const &refs_tensor)
{

  // cache[i] = (beta1_t_ * cache[i]) + ((1.0 - beta1_t_) * (input_gradients[i]/batch_size));
  fetch::math::Multiply(refs_tensor,
                        (static_cast<DataType>(1.0) - beta1_t_) / static_cast<DataType>(batch_size),
                        gradient_tensor);
  fetch::math::Multiply(cache_tensor, beta1_t_, cache_tensor);
  fetch::math::Add(cache_tensor, gradient_tensor, cache_tensor);

  // mt   = cache[i] / (1.0 - beta1_t_);
  fetch::math::Divide(cache_tensor, (static_cast<DataType>(1.0) - beta1_t_), mt_tensor);

  // momentum[i] = (beta2_t_ * momentum[i]) + ((1.0 - beta2_t_) *
  // ((input_gradients[i]/batch_size)^2));
  fetch::math::Divide(refs_tensor, static_cast<DataType>(batch_size), v_tensor);
  fetch::math::Square(v_tensor, v_tensor);
  fetch::math::Multiply(v_tensor, (static_cast<DataType>(1.0) - beta2_t_), v_tensor);
  fetch::math::Multiply(momentum_tensor, beta2_t_, momentum_tensor);
  fetch::math::Add(momentum_tensor, v_tensor, momentum_tensor);

  // vt   = momentum[i] / (1.0 - beta2_t_);
  fetch::math::Divide(momentum_tensor, (static_cast<DataType>(1.0) - beta2_t_), v_tensor);

  // output_gradients[i] = -this->learning_rate_ * mt / (sqrt(vt) + epsilon_);
  fetch::math::Sqrt(v_tensor, gradient_tensor);
  fetch::math::Add(gradient_tensor, epsilon_, gradient_tensor);
  fetch::math::Divide(mt_tensor, gradient_tensor, gradient_tensor);
  fetch::math::Multiply(gradient_tensor, -this->learning_rate_, gradient_tensor);
}

template <class T>
void LazyAdamOptimiser<T>::ApplyGradients(SizeType batch_size)
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

  std::vector<SizeSet> rows;

  while (gradient_it != this->gradients_.end())
  {
    // Skip frozen trainables
    if ((*trainable_it)->GetFrozenState())
    {
      continue;
    }

    std::pair<TensorType const, SizeSet const> gradient_pair =
        (*trainable_it)->GetSparseGradientsReferences();
    rows.push_back(gradient_pair.second);

    // Normal ApplyGradient
    // if number_of_rows_to_update*sparsity_threshold_ > total_rows
    if (rows.at(rows.size() - 1).empty() ||
        (rows.at(rows.size() - 1).size() * sparsity_threshold_) > gradient_pair.first.shape().at(1))
    {
      ApplyLogic(batch_size, *gradient_it, *momentum_it, *mt_it, *vt_it, *cached_weight_it,
                 (*trainable_it)->GetGradientsReferences());
    }

    else
    {
      // Sparse apply gradient
      // if number_of_rows_to_update*sparsity_threshold_ <= total_rows

      for (SizeType update_index : rows.at(rows.size() - 1))
      {

        auto       gradient_slice        = gradient_it->Slice(update_index, 1);
        TensorType gradient_slice_tensor = gradient_slice.Copy();

        auto       momentum_slice        = momentum_it->Slice(update_index, 1);
        TensorType momentum_slice_tensor = momentum_slice.Copy();

        auto       mt_slice        = mt_it->Slice(update_index, 1);
        TensorType mt_slice_tensor = mt_slice.Copy();

        auto       v_slice        = vt_it->Slice(update_index, 1);
        TensorType v_slice_tensor = v_slice.Copy();

        auto       cache_slice        = cached_weight_it->Slice(update_index, 1);
        TensorType cache_slice_tensor = cache_slice.Copy();

        auto       refs_slice = (*trainable_it)->GetGradientsReferences().Slice(update_index, 1);
        TensorType refs_slice_tensor = refs_slice.Copy();

        ApplyLogic(batch_size, gradient_slice_tensor, momentum_slice_tensor, mt_slice_tensor,
                   v_slice_tensor, cache_slice_tensor, refs_slice_tensor);

        // Put things back
        gradient_slice.Assign(gradient_slice_tensor);
        momentum_slice.Assign(momentum_slice_tensor);
        mt_slice.Assign(mt_slice_tensor);
        v_slice.Assign(v_slice_tensor);
        cache_slice.Assign(cache_slice_tensor);
      }

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
  this->graph_->ApplySparseGradients(this->gradients_, rows);
}

template <class T>
void LazyAdamOptimiser<T>::ResetCache()
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
struct MapSerializer<ml::optimisers::LazyAdamOptimiser<TensorType>, D>
{
  using Type                          = ml::optimisers::LazyAdamOptimiser<TensorType>;
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
