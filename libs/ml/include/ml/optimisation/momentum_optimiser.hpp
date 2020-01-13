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
#include "ml/optimisation/optimiser.hpp"

namespace fetch {
namespace ml {
namespace optimisers {

/**
 * Adaptive Momentum optimiser
 * i.e.  Stochastic gradient descent with momentum
 * @tparam T TensorType
 * @tparam C CriterionType
 */
template <class T>
class MomentumOptimiser : public Optimiser<T>
{
public:
  using TensorType = T;
  using DataType   = typename TensorType::Type;
  using SizeType   = fetch::math::SizeType;

  MomentumOptimiser(std::shared_ptr<Graph<T>>       graph,
                    std::vector<std::string> const &input_node_names,
                    std::string const &label_node_name, std::string const &output_node_name,
                    DataType const &learning_rate   = fetch::math::Type<DataType>("0.001"),
                    DataType const &momentum_update = fetch::math::Type<DataType>("0.9"));

  MomentumOptimiser(std::shared_ptr<Graph<T>>       graph,
                    std::vector<std::string> const &input_node_names,
                    std::string const &label_node_name, std::string const &output_node_name,
                    fetch::ml::optimisers::LearningRateParam<DataType> const &learning_rate_param,
                    DataType const &momentum_update = fetch::math::Type<DataType>("0.9"));

  ~MomentumOptimiser() override = default;

  OptimiserType OptimiserCode() override
  {
    return OptimiserType::MOMENTUM;
  }

private:
  std::vector<TensorType> momentum_;
  DataType                momentum_update_;
  DataType                negative_one_{-1};
  DataType                zero_{0};

  void ApplyGradients(SizeType batch_size) override;
  void ResetMomentum();
  void Init();
};

}  // namespace optimisers
}  // namespace ml
}  // namespace fetch
