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
#include "ml/optimisation/types.hpp"

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
  using OptimiserType    = fetch::ml::optimisers::OptimiserType;
  //  using OptimiserType    = fetch::ml::optimisers::AdamOptimiser<TensorType, CostFunctionType>;

  DNNClassifier(EstimatorConfig<DataType>                           estimator_config,
                std::shared_ptr<DataLoader<TensorType, TensorType>> data_loader_ptr,
                std::vector<SizeType> const &                       hidden_layers,
                OptimiserType optimiser_type = OptimiserType::ADAM);

  // construction setup methods
  void SetupModel(std::vector<SizeType> const &hidden_layers);
  //  void SetupOptimiser();

  // primary run method
  bool Run(SizeType n_steps, RunMode const &rm) override;

private:
  std::shared_ptr<DataLoader<TensorType, TensorType>> data_loader_ptr_;

  std::shared_ptr<optimisers::Optimiser<TensorType, CostFunctionType>> optimiser_ptr_;
  //  OptimiserType optimiser_;

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
    std::shared_ptr<DataLoader<TensorType, TensorType>> data_loader_ptr,
    std::vector<SizeType> const &hidden_layers, OptimiserType optimiser_type)
  : Estimator<TensorType>(estimator_config)
  , data_loader_ptr_(data_loader_ptr)
{

  assert(hidden_layers.size() > 0);

  // instantiate feed forward network graph
  SetupModel(hidden_layers);

  // instantiate optimiser
  auto input = {input_};
  if (!(fetch::ml::optimisers::AddOptimiser<TensorType, CostFunctionType>(
          optimiser_type, optimiser_ptr_, this->graph_ptr_, input, output_,
          this->estimator_config_.learning_rate)))
  {
    throw std::runtime_error("DNNClassifier initialised with unrecognised optimiser");
  }
}

/**
 * Sets up the neural net classifier architecture
 * @tparam TensorType
 * @param hidden_layers the input dimensions for all layers
 */
template <typename TensorType>
void DNNClassifier<TensorType>::SetupModel(std::vector<SizeType> const &hidden_layers)
{
  input_ = this->graph_ptr_->template AddNode<ops::PlaceHolder<TensorType>>("Input", {});
  std::string cur_input = input_;
  for (SizeType cur_layer = 1; cur_layer < hidden_layers.size() - 1; ++cur_layer)
  {
    cur_input = this->graph_ptr_->template AddNode<layers::FullyConnected<TensorType>>(
        "", {cur_input}, hidden_layers.at(cur_layer - 1), hidden_layers.at(cur_layer),
        fetch::ml::details::ActivationType::RELU);
  }
  output_ = this->graph_ptr_->template AddNode<layers::FullyConnected<TensorType>>(
      "Output", {cur_input}, hidden_layers.at(hidden_layers.size() - 2),
      hidden_layers.at(hidden_layers.size() - 1), fetch::ml::details::ActivationType::SOFTMAX);
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
