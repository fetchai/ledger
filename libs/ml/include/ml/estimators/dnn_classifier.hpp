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

  DNNClassifier(EstimatorConfig<DataType>                                        estimator_config,
                std::shared_ptr<dataloaders::DataLoader<TensorType, TensorType>> data_loader_ptr,
                std::vector<SizeType> const &                                    hidden_layers,
                OptimiserType optimiser_type = OptimiserType::ADAM);

  void SetupModel(std::vector<SizeType> const &hidden_layers);

  virtual bool Train(SizeType n_steps) override;
  virtual bool Train(SizeType n_steps, DataType &loss) override;
  virtual bool Validate() override;
  virtual bool Predict(TensorType &input, TensorType &output) override;

private:
  std::shared_ptr<dataloaders::DataLoader<TensorType, TensorType>> data_loader_ptr_;
  std::shared_ptr<optimisers::Optimiser<TensorType>>               optimiser_ptr_;

  std::string input_;
  std::string label_;
  std::string output_;
  std::string error_;

  void PrintStats(SizeType epoch, DataType loss);
};

/**
 * constructor sets up the neural net architecture and optimiser
 * @tparam TensorType
 * @param estimator_config config parameters for setting up the network
 * @param data_loader_ptr pointer to the dataloader for running the optimiser
 * @param hidden_layers vector of hidden layers for defining the architecture
 * @param optimiser_type type of optimiser to run
 */
template <typename TensorType>
DNNClassifier<TensorType>::DNNClassifier(
    EstimatorConfig<DataType>                                        estimator_config,
    std::shared_ptr<dataloaders::DataLoader<TensorType, TensorType>> data_loader_ptr,
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
          optimiser_type, optimiser_ptr_, this->graph_ptr_, input, label_, error_,
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

  label_ = this->graph_ptr_->template AddNode<ops::PlaceHolder<TensorType>>("Label", {});
  error_ = this->graph_ptr_->template AddNode<CostFunctionType>("Error", {output_, label_});
}

/**
 * An interface to train that doesn't report loss
 * @tparam TensorType
 * @param n_steps
 * @return
 */
template <typename TensorType>
bool DNNClassifier<TensorType>::Train(SizeType n_steps)
{
  DataType _;
  return DNNClassifier<TensorType>::Train(n_steps, _);
}

template <typename TensorType>
bool DNNClassifier<TensorType>::Train(SizeType n_steps, DataType &loss)
{
  loss = DataType{0};
  DataType min_loss;
  SizeType patience_count{0};
  bool     stop_early = false;

  // run for one epoch
  loss     = optimiser_ptr_->Run(*data_loader_ptr_, this->estimator_config_.batch_size,
                             this->estimator_config_.subset_size);
  min_loss = loss;

  // run for remaining epochs with early stopping
  SizeType step{1};
  while ((!stop_early) && (step < n_steps))
  {
    if (this->estimator_config_.print_stats)
    {
      PrintStats(step, loss);
    }

    // run optimiser for one epoch
    loss = optimiser_ptr_->Run(*data_loader_ptr_, this->estimator_config_.batch_size,
                               this->estimator_config_.subset_size);

    // update early stopping
    if (this->estimator_config_.early_stopping)
    {
      if (loss < (min_loss - this->estimator_config_.min_delta))
      {
        min_loss       = loss;
        patience_count = 0;
      }
      else
      {
        patience_count++;
      }

      if (patience_count >= this->estimator_config_.patience)
      {
        stop_early = true;
      }
    }

    step++;
  }
  return true;
}

template <typename TensorType>
bool DNNClassifier<TensorType>::Validate()
{
  throw std::runtime_error("validate not implemented");
  return true;
}

template <typename TensorType>
bool DNNClassifier<TensorType>::Predict(TensorType &input, TensorType &output)
{
  this->graph_ptr_->SetInput(input_, input);
  output = this->graph_ptr_->Evaluate(output_);

  return true;
}

template <typename TensorType>
void DNNClassifier<TensorType>::PrintStats(SizeType epoch, DataType loss)
{
  std::cout << "epoch: " << epoch << ", loss: " << loss << std::endl;
}

}  // namespace estimator
}  // namespace ml
}  // namespace fetch
