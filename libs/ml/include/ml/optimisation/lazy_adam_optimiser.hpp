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

#include "ml/core/graph.hpp"
#include "ml/optimisation/adam_optimiser.hpp"

namespace fetch {
namespace ml {
namespace optimisers {

/**
 * LazyAdam is a variant of the Adam optimizer that handles sparse updates more efficiently.
 * The original Adam algorithm maintains moving-average accumulators for each trainable variable;
 * the accumulators are updated at every step. This class provides lazier handling of gradient
 * updates for sparse variables. It only updates moving-average accumulators for sparse variable
 * indices that appear in the current batch, rather than updating the accumulators for all indices.
 * Compared with the original Adam optimizer, it can provide large improvements in model training
 * throughput for some applications. However, it provides slightly different semantics than the
 * original Adam algorithm, and may lead to different empirical results.
 * ref: https://www.tensorflow.org/addons/tutorials/optimizers_lazyadam
 * @tparam T TensorType
 */
template <class T>
class LazyAdamOptimiser : public AdamOptimiser<T>
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
                    DataType const &learning_rate      = fetch::math::Type<DataType>("0.001"),
                    DataType const &beta1              = fetch::math::Type<DataType>("0.9"),
                    DataType const &beta2              = fetch::math::Type<DataType>("0.999"),
                    SizeType        sparsity_threshold = 2,
                    DataType const &epsilon            = fetch::math::Type<DataType>("0.0001"));

  LazyAdamOptimiser(std::shared_ptr<Graph<T>>       graph,
                    std::vector<std::string> const &input_node_names,
                    std::string const &label_node_name, std::string const &output_node_name,
                    fetch::ml::optimisers::LearningRateParam<DataType> const &learning_rate_param,
                    DataType const &beta1              = fetch::math::Type<DataType>("0.9"),
                    DataType const &beta2              = fetch::math::Type<DataType>("0.999"),
                    SizeType        sparsity_threshold = 2,
                    DataType const &epsilon            = fetch::math::Type<DataType>("0.0001"));

  ~LazyAdamOptimiser() override = default;

  void ApplyGradients(SizeType batch_size) override;

  template <typename X, typename D>
  friend struct serializers::MapSerializer;

  OptimiserType OptimiserCode() override
  {
    return OptimiserType::LAZY_ADAM;
  }

private:
  // ApplyGradientSparse if number_of_rows_to_update * sparsity_threshold_ <= total_rows
  // Current value was empirically derived from ml/benchmarks/embeddings benchmark results
  SizeType sparsity_threshold_ = 2;

  void ApplyLogic(SizeType batch_size, TensorType &gradient_tensor, TensorType &momentum_tensor,
                  TensorType &mt_tensor, TensorType &v_tensor, TensorType &cache_tensor,
                  TensorType const &refs_tensor);
};

}  // namespace optimisers
}  // namespace ml

namespace serializers {
/**
 * serializer for LazyAdamOptimiser
 * @tparam TensorType
 */
template <typename TensorType, typename D>
struct MapSerializer<ml::optimisers::LazyAdamOptimiser<TensorType>, D>
{
  using Type                              = ml::optimisers::LazyAdamOptimiser<TensorType>;
  using DriverType                        = D;
  static uint8_t const BASE_OPTIMISER     = 1;
  static uint8_t const CACHE              = 2;
  static uint8_t const MOMENTUM           = 3;
  static uint8_t const MT                 = 4;
  static uint8_t const VT                 = 5;
  static uint8_t const BETA1              = 6;
  static uint8_t const BETA2              = 7;
  static uint8_t const BETA1_T            = 8;
  static uint8_t const BETA2_T            = 9;
  static uint8_t const SPARSITY_THRESHOLD = 10;
  static uint8_t const EPSILON            = 11;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &sp)
  {
    auto map = map_constructor(11);

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
    map.Append(SPARSITY_THRESHOLD, sp.sparsity_threshold);
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
    map.ExpectKeyGetValue(SPARSITY_THRESHOLD, sp.sparsity_threshold);
    map.ExpectKeyGetValue(EPSILON, sp.epsilon_);
  }
};
}  // namespace serializers

}  // namespace fetch
