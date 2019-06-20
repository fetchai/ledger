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

//#include "ml/meta/ml_type_traits.hpp"

#include "ml/graph.hpp"
#include "ml/ops/loss_functions/types.hpp"
#include "ml/optimisation/optimiser.hpp"
#include "ml/optimisation/types.hpp"

namespace fetch {
namespace ml {
namespace estimator {

template <typename DataType>
struct EstimatorConfig
{
  using SizeType         = fetch::math::SizeType;
  using OptimiserType    = fetch::ml::optimisers::OptimiserType;
  using CostFunctionType = fetch::ml::ops::CostFunctionType;

  EstimatorConfig() = default;

  bool     early_stopping = false;
  SizeType patience       = 10;
  DataType min_delta      = DataType(0.0);

  DataType learning_rate = DataType(0.001);
  DataType epoch_decay   = DataType(0.9);

  SizeType batch_size  = SizeType(32);
  SizeType subset_size = fetch::ml::optimisers::SIZE_NOT_SET;

  OptimiserType    opt  = OptimiserType::ADAM;
  CostFunctionType cost = CostFunctionType::SOFTMAX_CROSS_ENTROPY;
};

template <typename TensorType>
class Estimator
{
public:
  using DataType = typename TensorType::Type;
  using SizeType = fetch::math::SizeType;

  Estimator(EstimatorConfig<DataType> estimator_config = EstimatorConfig<DataType>())
    : estimator_config_(estimator_config)
  {}

  virtual ~Estimator() = default;

  virtual bool Train(SizeType n_steps)    = 0;
  virtual bool Validate()                 = 0;
  virtual bool Predict(TensorType &input) = 0;

protected:
  EstimatorConfig<DataType>          estimator_config_;
  std::shared_ptr<Graph<TensorType>> graph_ptr_ = std::make_shared<Graph<TensorType>>();
};

}  // namespace estimator
}  // namespace ml
}  // namespace fetch
