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

#include "ml/optimisation/adam_optimiser.hpp"

namespace fetch {
namespace ml {
namespace optimisers {

template <typename T>
struct LearningRateParam;

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

  inline OptimiserType OptimiserCode() override
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
}  // namespace fetch
