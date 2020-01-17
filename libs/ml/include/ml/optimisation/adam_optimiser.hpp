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

#include "ml/optimisation/optimiser.hpp"

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

  inline OptimiserType OptimiserCode() override
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

}  // namespace optimisers
}  // namespace ml

}  // namespace fetch
