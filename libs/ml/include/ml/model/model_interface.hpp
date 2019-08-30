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

#include "ml/core/graph.hpp"
#include "ml/ops/loss_functions/types.hpp"
#include "ml/optimisation/optimiser.hpp"
#include "ml/optimisation/types.hpp"

namespace fetch {
namespace ml {
namespace model {

template <typename TensorType>
class ModelInterface
{
public:
  using DataType          = typename TensorType::Type;
  using SizeType          = fetch::math::SizeType;
  using GraphType         = Graph<TensorType>;
  using DataLoaderType    = dataloaders::DataLoader<TensorType, TensorType>;
  using OptimiserType     = optimisers::Optimiser<TensorType>;
  using OptimiserTypeType = optimisers::OptimiserType;
  using GraphPtrType      = typename std::shared_ptr<GraphType>;
  using DataLoaderPtrType = typename std::shared_ptr<DataLoaderType>;
  using OptimiserPtrType  = typename std::shared_ptr<OptimiserType>;

  virtual bool Train(SizeType n_steps);
  virtual bool Train(SizeType n_steps, DataType &loss);
  virtual bool Test(DataType &test_loss);
  virtual bool Predict(TensorType &input, TensorType &output);

  explicit ModelInterface(DataLoaderPtrType dataloader_ptr, OptimiserTypeType optimiser_type,
                          ModelConfig<DataType> const &model_config = ModelConfig<DataType>())
    : model_config_(model_config)
    , dataloader_ptr_(dataloader_ptr)
    , optimiser_type_(optimiser_type){};

protected:
  ModelConfig<DataType> model_config_;
  GraphPtrType          graph_ptr_ = std::make_shared<GraphType>();
  DataLoaderPtrType     dataloader_ptr_;
  OptimiserPtrType      optimiser_ptr_;
  OptimiserTypeType     optimiser_type_;

  std::string input_;
  std::string label_;
  std::string output_;
  std::string error_;

  bool optimiser_set_ = false;

  virtual void PrintStats(SizeType epoch, DataType loss,
                          DataType test_loss = fetch::math::numeric_max<DataType>());

private:
  bool SetOptimiser();
};

/**
 * An interface to train that doesn't report loss
 * @tparam TensorType
 * @param n_steps
 * @return
 */
template <typename TensorType>
bool ModelInterface<TensorType>::Train(SizeType n_steps)
{
  DataType _;
  return ModelInterface<TensorType>::Train(n_steps, _);
}

template <typename TensorType>
bool ModelInterface<TensorType>::Train(SizeType n_steps, DataType &loss)
{
  if (!SetOptimiser())
  {
    throw std::runtime_error("failed to set up  optimiser");
  }

  dataloader_ptr_->SetMode(dataloaders::DataLoaderMode::TRAIN);

  loss               = DataType{0};
  DataType min_loss  = fetch::math::numeric_max<DataType>();
  DataType test_loss = fetch::math::numeric_max<DataType>();
  SizeType patience_count{0};
  bool     stop_early = false;

  // run for one epoch
  loss     = optimiser_ptr_->Run(*dataloader_ptr_, this->model_config_.batch_size,
                             this->model_config_.subset_size);
  min_loss = loss;

  // run for remaining epochs with early stopping
  SizeType step{1};
  while ((!stop_early) && (step < n_steps))
  {
    if (this->model_config_.print_stats)
    {
      if (this->model_config_.test)
      {
        Test(test_loss);
      }
      PrintStats(step, loss, test_loss);
    }

    // run optimiser for one epoch
    loss = optimiser_ptr_->Run(*dataloader_ptr_, this->model_config_.batch_size,
                               this->model_config_.subset_size);

    // update early stopping
    if (this->model_config_.early_stopping)
    {
      if (loss < (min_loss - this->model_config_.min_delta))
      {
        min_loss       = loss;
        patience_count = 0;
      }
      else
      {
        patience_count++;
      }

      if (patience_count >= this->model_config_.patience)
      {
        stop_early = true;
      }
    }

    step++;
  }
  return true;
}

template <typename TensorType>
bool ModelInterface<TensorType>::Test(DataType &test_loss)
{
  if (!SetOptimiser())
  {
    throw std::runtime_error("failed to set up  optimiser");
  }

  dataloader_ptr_->SetMode(dataloaders::DataLoaderMode::TEST);

  SizeType test_set_size = dataloader_ptr_->Size();

  dataloader_ptr_->Reset();
  bool is_done_set;
  auto test_pair = dataloader_ptr_->PrepareBatch(test_set_size, is_done_set);

  this->graph_ptr_->SetInput(label_, test_pair.first);
  this->graph_ptr_->SetInput(input_, test_pair.second.at(0));
  test_loss = *(this->graph_ptr_->ForwardPropagate(error_).begin());

  return true;
}

template <typename TensorType>
bool ModelInterface<TensorType>::Predict(TensorType &input, TensorType &output)
{
  if (!SetOptimiser())
  {
    throw std::runtime_error("failed to set up optimiser");
  }

  this->graph_ptr_->SetInput(input_, input);
  output = this->graph_ptr_->Evaluate(output_);

  return true;
}

template <typename TensorType>
void ModelInterface<TensorType>::PrintStats(SizeType epoch, DataType loss, DataType test_loss)
{
  if (this->model_config_.test)
  {
    std::cout << "epoch: " << epoch << ", loss: " << loss << ", test loss: " << test_loss
              << std::endl;
  }
  else
  {
    std::cout << "epoch: " << epoch << ", loss: " << loss << std::endl;
  }
}

/**
 * The optimiser has to be set with a constructed graph. So this must be called after
 * the concrete model has finished set up. Since ModelInterface doesn't know when this
 * will occur; we just check for the flag before training or testing and set up if
 * needed
 * @return
 */
template <typename TensorType>
bool ModelInterface<TensorType>::SetOptimiser()
{
  if (!optimiser_set_)
  {
    // instantiate optimiser
    if (!(fetch::ml::optimisers::AddOptimiser<TensorType>(
            optimiser_type_, this->optimiser_ptr_, this->graph_ptr_,
            std::vector<std::string>{this->input_}, this->label_, this->error_,
            this->model_config_.learning_rate_param)))
    {
      throw std::runtime_error("DNNClassifier initialised with unrecognised optimiser");
    }
    optimiser_set_ = true;
  }
  return true;
}

}  // namespace model
}  // namespace ml
}  // namespace fetch
