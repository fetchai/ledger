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

#include "math/base_types.hpp"

#include "ml/layers/fully_connected.hpp"
#include "ml/ops/placeholder.hpp"

#include "ml/ops/loss_functions/cross_entropy.hpp"

#include "ml/dataloaders/dataloader.hpp"
#include "ml/estimators/estimator.hpp"
#include "ml/optimisation/adam_optimiser.hpp"

namespace fetch {
namespace ml {
namespace estimator {

template <typename TensorType>
class DNNClassifier : public Estimator<TensorType>
{
public:
  using SizeType         = fetch::math::SizeType;
  using DataType         = typename TensorType::Type;
  using CostFunctionType = fetch::ml::ops::CrossEntropy<TensorType>;
  using OptimiserType    = fetch::ml::optimisers::AdamOptimiser<TensorType, CostFunctionType>;

  //  DNNClassifier(EstimatorConfig<DataType> estimator_config,
  //  std::shared_ptr<DataLoader<TensorType, TensorType>> data_loader_ptr) :
  //  Estimator(estimator_config), data_loader_ptr(data_loader_ptr_);
  DNNClassifier(EstimatorConfig<DataType>                           estimator_config,
                std::shared_ptr<DataLoader<TensorType, TensorType>> data_loader_ptr);

  // construction setup methods
  void SetupModel();
  //  void SetupOptimiser();

  // primary run method
  bool Run(SizeType n_steps, RunMode const &rm) override;

private:
  std::shared_ptr<DataLoader<TensorType, TensorType>> data_loader_ptr_;

  std::shared_ptr<OptimiserType> optimiser_ptr_;
  //  OptimiserType optimiser_;

  // TODO - implement model_function to use arbitrary neural nets
  // TODO - implement arbitrary dataloader
  // TODO - implement arbitrary optimiser

  std::string input_;
  std::string layer_1_;
  std::string layer_2_;
  std::string output_;
};

/**
 *
 * @tparam TensorType
 */
template <typename TensorType>
DNNClassifier<TensorType>::DNNClassifier(
    EstimatorConfig<DataType>                           estimator_config,
    std::shared_ptr<DataLoader<TensorType, TensorType>> data_loader_ptr)
  : Estimator<TensorType>(estimator_config)
  , data_loader_ptr_(data_loader_ptr)
{
  // instantiate feed forward network graph
  SetupModel();

  optimiser_ptr_.reset(new OptimiserType(this->graph_ptr_, {input_}, output_,
                                         this->estimator_config_.learning_rate));
}

/**
 *
 * @tparam TensorType
 */
template <typename TensorType>
void DNNClassifier<TensorType>::SetupModel()
{
  input_   = this->graph_ptr_->template AddNode<ops::PlaceHolder<TensorType>>("Input", {});
  layer_1_ = this->graph_ptr_->template AddNode<layers::FullyConnected<TensorType>>(
      "FC1", {input_}, 28u * 28u, 10u, fetch::ml::details::ActivationType::RELU);
  layer_2_ = this->graph_ptr_->template AddNode<layers::FullyConnected<TensorType>>(
      "FC2", {layer_1_}, 10u, 10u, fetch::ml::details::ActivationType::RELU);
  output_ = this->graph_ptr_->template AddNode<layers::FullyConnected<TensorType>>(
      "FC3", {layer_2_}, 10u, 10u, fetch::ml::details::ActivationType::SOFTMAX);
}

/**
 *
 * @param steps
 * @param run_mode
 * @return
 */
template <typename TensorType>
bool DNNClassifier<TensorType>::Run(SizeType n_steps, RunMode const &run_mode)
{
  switch (run_mode)
  {
  case (RunMode::TRAIN):
  {
    DataType loss;
    for (SizeType i{0}; i < n_steps; i++)
    {
      loss = optimiser_ptr_->Run(*data_loader_ptr_, this->estimator_config_.batch_size);
      std::cout << "Loss: " << loss << std::endl;
    }
    return true;
  }
  case (RunMode::VALIDATE):
  {
    return false;
    break;
  }
  case (RunMode::PREDICT):
  {
    return false;
    break;
  }
  default:
  {
    return false;
  }
  }
}

}  // namespace estimator
}  // namespace ml
}  // namespace fetch
