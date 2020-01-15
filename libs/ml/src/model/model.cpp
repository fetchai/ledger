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

#include "ml/model/model.hpp"
#include "ml/ops/metrics/types.hpp"
#include "ml/utilities/graph_saver.hpp"

namespace fetch {
namespace ml {
namespace model {

template <typename TensorType>
Model<TensorType>::Model(const Model &other)
{
  graph_ptr_.reset(other.graph_ptr_.get());
  dataloader_ptr_.reset(other.dataloader_ptr_.get());
  optimiser_ptr_.reset(other.optimiser_ptr_.get());
}

template <typename TensorType>
void Model<TensorType>::Compile(OptimiserType optimiser_type, ops::LossType loss_type,
                                std::vector<ops::MetricType> const &metrics)
{
  // add loss to graph
  if (!loss_set_)
  {
    switch (loss_type)
    {
    case (ops::LossType::CROSS_ENTROPY):
    {
      error_ = graph_ptr_->template AddNode<ops::CrossEntropyLoss<TensorType>>("Error",
                                                                               {output_, label_});
      break;
    }
    case (ops::LossType::MEAN_SQUARE_ERROR):
    {
      error_ = graph_ptr_->template AddNode<ops::MeanSquareErrorLoss<TensorType>>(
          "Error", {output_, label_});
      break;
    }
    case (ops::LossType::SOFTMAX_CROSS_ENTROPY):
    {
      error_ = graph_ptr_->template AddNode<ops::SoftmaxCrossEntropyLoss<TensorType>>(
          "Error", {output_, label_});
      break;
    }
    case (ops::LossType::NONE):
    {
      throw ml::exceptions::InvalidMode(
          "must set loss function on model compile for this model type");
    }
    default:
    {
      throw ml::exceptions::InvalidMode("unrecognised loss type in model compilation");
    }
    }
  }
  else
  {
    if (loss_type != ops::LossType::NONE)
    {
      throw ml::exceptions::InvalidMode(
          "attempted to set loss function on compile but loss function already previously set! "
          "maybe using wrong type of model?");
    }
  }

  // Add all the metric nodes to the graph and store the names in metrics_ for future reference
  for (auto const &met : metrics)
  {
    switch (met)
    {
    case (ops::MetricType::CATEGORICAL_ACCURACY):
    {
      metrics_.emplace_back(graph_ptr_->template AddNode<ops::CategoricalAccuracy<TensorType>>(
          "", {output_, label_}));
      break;
    }
    case ops::MetricType::CROSS_ENTROPY:
    {
      metrics_.emplace_back(
          graph_ptr_->template AddNode<ops::CrossEntropyLoss<TensorType>>("", {output_, label_}));
      break;
    }
    case ops::MetricType::MEAN_SQUARE_ERROR:
    {
      metrics_.emplace_back(graph_ptr_->template AddNode<ops::MeanSquareErrorLoss<TensorType>>(
          "", {output_, label_}));
      break;
    }
    case ops::MetricType::SOFTMAX_CROSS_ENTROPY:
    {
      metrics_.emplace_back(graph_ptr_->template AddNode<ops::SoftmaxCrossEntropyLoss<TensorType>>(
          "", {output_, label_}));
      break;
    }
    default:
    {
      throw ml::exceptions::InvalidMode("unrecognised metric type in model compilation");
    }
    }
  }

  // set the optimiser
  if (!optimiser_set_)
  {
    if (!(fetch::ml::optimisers::AddOptimiser<TensorType>(
            optimiser_type, optimiser_ptr_, graph_ptr_, std::vector<std::string>{input_}, label_,
            error_, model_config_.learning_rate_param)))
    {
      throw ml::exceptions::InvalidMode("DNNClassifier initialised with unrecognised optimiser");
    }
    optimiser_set_ = true;
  }

  compiled_ = true;
}

/**
 * An interface to train that trains for one epoch
 * @tparam TensorType
 * @return
 */
template <typename TensorType>
void Model<TensorType>::Train()
{
  model_config_.subset_size = fetch::ml::optimisers::SIZE_NOT_SET;
  DataType _;
  Model<TensorType>::TrainImplementation(_);
}

/**
 * An interface to train that doesn't report loss
 * @tparam TensorType
 * @param n_rounds
 * @return
 */
template <typename TensorType>
void Model<TensorType>::Train(SizeType n_rounds)
{
  DataType _;
  Model<TensorType>::Train(n_rounds, _);
}

template <typename TensorType>
void Model<TensorType>::Train(SizeType n_rounds, DataType &loss)
{
  TrainImplementation(loss, n_rounds);
}

template <typename TensorType>
void Model<TensorType>::Test(DataType &test_loss)
{
  if (!compiled_)
  {
    throw ml::exceptions::InvalidMode("must compile model before testing");
  }
  if (!DataLoaderIsSet())
  {
    throw ml::exceptions::InvalidMode("must set data before testing");
  }

  dataloader_ptr_->SetMode(dataloaders::DataLoaderMode::TEST);

  SizeType test_set_size = dataloader_ptr_->Size();

  dataloader_ptr_->Reset();
  bool is_done_set;
  auto test_pair = dataloader_ptr_->PrepareBatch(test_set_size, is_done_set);

  this->graph_ptr_->SetInput(label_, test_pair.first);
  this->graph_ptr_->SetInput(input_, test_pair.second.at(0));
  test_loss = *(this->graph_ptr_->Evaluate(error_).begin());
}

template <typename TensorType>
void Model<TensorType>::Predict(TensorType &input, TensorType &output)
{
  if (!compiled_)
  {
    throw ml::exceptions::InvalidMode("must compile model before predicting");
  }

  this->graph_ptr_->SetInput(input_, input);
  output = this->graph_ptr_->Evaluate(output_);
}

template <typename TensorType>
typename Model<TensorType>::DataVectorType Model<TensorType>::Evaluate(
    dataloaders::DataLoaderMode dl_mode, SizeType batch_size)
{
  if (!compiled_)
  {
    throw ml::exceptions::InvalidMode("must compile model before evaluating");
  }
  if (!DataLoaderIsSet())
  {
    throw ml::exceptions::InvalidMode("must set data before evaluating");
  }

  dataloader_ptr_->SetMode(dl_mode);
  dataloader_ptr_->SetRandomMode(false);
  bool is_done_set;
  if (batch_size == 0)
  {
    batch_size = dataloader_ptr_->Size();
  }
  auto evaluate_pair = dataloader_ptr_->PrepareBatch(batch_size, is_done_set);

  this->graph_ptr_->SetInput(label_, evaluate_pair.first);
  this->graph_ptr_->SetInput(input_, evaluate_pair.second.at(0));

  DataVectorType ret;
  // call evaluate on the graph and store the loss in ret
  ret.emplace_back(*(this->graph_ptr_->Evaluate(error_).cbegin()));

  // push further metrics into ret - due to graph caching these subsequent evaluate calls are cheap
  for (auto const &met : metrics_)
  {
    ret.emplace_back(*(this->graph_ptr_->Evaluate(met).cbegin()));
  }
  return ret;
}

template <typename TensorType>
void Model<TensorType>::UpdateConfig(ModelConfig<DataType> &model_config)
{
  model_config_ = model_config;
}

/**
 * Overwrite the models dataloader with an external custom dataloader.
 * @tparam TensorType
 * @param dataloader_ptr
 */
template <typename TensorType>
void Model<TensorType>::SetDataloader(std::shared_ptr<DataLoaderType> dataloader_ptr)
{
  dataloader_ptr_ = dataloader_ptr;
}

/**
 * returns a pointer to the models dataloader
 * @return
 */
template <typename TensorType>
std::shared_ptr<const typename Model<TensorType>::DataLoaderType> Model<TensorType>::GetDataloader()
{
  return dataloader_ptr_;
}

/**
 * returns a pointer to the models optimiser
 * @return
 */
template <typename TensorType>
std::shared_ptr<const typename Model<TensorType>::ModelOptimiserType>
Model<TensorType>::GetOptimiser()
{
  return optimiser_ptr_;
}

template <typename TensorType>
std::string Model<TensorType>::InputName()
{
  return input_;
}

template <typename TensorType>
std::string Model<TensorType>::LabelName()
{
  return label_;
}

template <typename TensorType>
std::string Model<TensorType>::OutputName()
{
  return output_;
}

template <typename TensorType>
std::string Model<TensorType>::ErrorName()
{
  return error_;
}

/// PROTECTED METHODS ///

template <typename TensorType>
void Model<TensorType>::PrintStats(SizeType epoch, DataType loss, DataType test_loss)
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

/// PRIVATE METHODS ///

template <typename TensorType>
void Model<TensorType>::TrainImplementation(DataType &loss, SizeType n_rounds)
{
  if (!compiled_)
  {
    throw ml::exceptions::InvalidMode("must compile model before training");
  }
  if (!DataLoaderIsSet())
  {
    throw ml::exceptions::InvalidMode("must set data before training");
  }

  dataloader_ptr_->SetMode(dataloaders::DataLoaderMode::TRAIN);

  DataType test_loss = fetch::math::numeric_max<DataType>();
  SizeType patience_count{0};
  bool     stop_early = false;

  // run for one subset - if this is not set it defaults to epoch
  loss_ =
      optimiser_ptr_->Run(*dataloader_ptr_, model_config_.batch_size, model_config_.subset_size);
  DataType min_loss = loss_;

  // run for remaining epochs (or subsets) with early stopping
  SizeType step{1};

  while ((!stop_early) && (step < n_rounds))
  {
    if (this->model_config_.print_stats)
    {
      if (this->model_config_.test)
      {
        Test(test_loss);
      }
      PrintStats(step, loss, test_loss);
    }

    if (this->model_config_.save_graph)
    {
      fetch::ml::utilities::SaveGraph(*graph_ptr_,
                                      model_config_.graph_save_location + std::to_string(step));
    }

    // run optimiser for one epoch (or subset)
    loss_ =
        optimiser_ptr_->Run(*dataloader_ptr_, model_config_.batch_size, model_config_.subset_size);

    // update early stopping
    if (this->model_config_.early_stopping)
    {
      if (loss_ < (min_loss - this->model_config_.min_delta))
      {
        min_loss       = loss_;
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

  loss = loss_;
}

template <typename TensorType>
bool Model<TensorType>::DataLoaderIsSet()
{
  if (!dataloader_ptr_)
  {
    return false;
  }
  return dataloader_ptr_->Size() != 0;
}

///////////////////////////////
/// EXPLICIT INSTANTIATIONS ///
///////////////////////////////
// ML-464
// template class Model<math::Tensor<int8_t>>;
template class Model<math::Tensor<int16_t>>;
template class Model<math::Tensor<int32_t>>;
template class Model<math::Tensor<int64_t>>;
template class Model<math::Tensor<float>>;
template class Model<math::Tensor<double>>;
template class Model<math::Tensor<fixed_point::fp32_t>>;
template class Model<math::Tensor<fixed_point::fp64_t>>;
template class Model<math::Tensor<fixed_point::fp128_t>>;
}  // namespace model
}  // namespace ml
}  // namespace fetch
