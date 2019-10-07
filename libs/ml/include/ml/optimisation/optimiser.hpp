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
#include "math/statistics/mean.hpp"
#include "ml/core/graph.hpp"
#include "ml/dataloaders/dataloader.hpp"
#include "ml/optimisation/learning_rate_params.hpp"
#include "ml/utilities/graph_builder.hpp"

#include <chrono>
#include <utility>

namespace fetch {
namespace ml {
namespace optimisers {

static constexpr fetch::math::SizeType SIZE_NOT_SET = fetch::math::numeric_max<math::SizeType>();

/**
 * Abstract gradient optimiser class
 * @tparam T TensorType
 * @tparam C CriterionType
 */
template <class T>
class Optimiser
{
public:
  using TensorType = T;
  using DataType   = typename TensorType::Type;
  using SizeType   = typename TensorType::SizeType;

  Optimiser() = default;
  Optimiser(std::shared_ptr<Graph<T>> graph, std::vector<std::string> input_node_names,
            std::string label_node_name, std::string output_node_name,
            DataType const &learning_rate = DataType(0.001));

  Optimiser(std::shared_ptr<Graph<T>> graph, std::vector<std::string> input_node_names,
            std::string label_node_name, std::string output_node_name,
            LearningRateParam<DataType> learning_rate_param);

  virtual ~Optimiser() = default;

  /// DATA RUN INTERFACES ///
  DataType Run(std::vector<TensorType> const &data, TensorType const &labels,
               SizeType batch_size = SIZE_NOT_SET);

  /// DATALOADER RUN INTERFACES ///
  DataType Run(fetch::ml::dataloaders::DataLoader<TensorType, TensorType> &loader,
               SizeType batch_size = SIZE_NOT_SET, SizeType subset_size = SIZE_NOT_SET);
  DataType Run(fetch::ml::dataloaders::DataLoader<TensorType, TensorType> &loader,
               LearningRateParam<DataType> learning_rate_param, SizeType batch_size = SIZE_NOT_SET,
               SizeType subset_size = SIZE_NOT_SET);

  void     UpdateLearningRate();
  void     IncrementEpochCounter();
  void     IncrementBatchCounters(SizeType batch_size);
  SizeType UpdateBatchSize(SizeType const &batch_size, SizeType const &data_size,
                           SizeType const &subset_size = SIZE_NOT_SET);

  std::shared_ptr<Graph<T>> GetGraph();
  virtual void              ApplyGradients(SizeType batch_size) = 0;

  template <typename X, typename D>
  friend struct serializers::MapSerializer;

protected:
  std::shared_ptr<Graph<T>> graph_;
  std::vector<std::string>  input_node_names_ = {};
  std::string               label_node_name_  = "";
  std::string               output_node_name_ = "";
  DataType                  learning_rate_    = fetch::math::numeric_max<DataType>();
  std::vector<std::shared_ptr<fetch::ml::ops::Trainable<TensorType>>> graph_trainables_;
  std::vector<TensorType>                                             gradients_;
  SizeType                                                            epoch_ = SIZE_NOT_SET;

private:
  DataType                                       loss_{};
  DataType                                       loss_sum_{};
  SizeType                                       step_{};
  SizeType                                       cumulative_step_ = 0;
  std::pair<TensorType, std::vector<TensorType>> input_;
  TensorType                                     cur_label_;
  TensorType                                     pred_label_;
  std::chrono::high_resolution_clock::time_point cur_time_;
  std::chrono::high_resolution_clock::time_point start_time_;
  std::chrono::duration<double>                  time_span_{};
  std::string                                    stat_string_;
  std::vector<TensorType>                        batch_data_;
  TensorType                                     batch_labels_;
  LearningRateParam<DataType>                    learning_rate_param_;

  void ResetGradients();

  void PrintStats(SizeType batch_size, SizeType subset_size);

  void Init();

  DataType RunImplementation(fetch::ml::dataloaders::DataLoader<TensorType, TensorType> &loader,
                             SizeType batch_size  = SIZE_NOT_SET,
                             SizeType subset_size = SIZE_NOT_SET);
};

template <class T>
void Optimiser<T>::Init()
{
  graph_->Compile();

  graph_trainables_ = graph_->GetTrainables();
  for (auto &train : graph_trainables_)
  {
    gradients_.emplace_back(TensorType(train->GetWeights().shape()));
  }
}

template <class T>
Optimiser<T>::Optimiser(std::shared_ptr<Graph<T>> graph, std::vector<std::string> input_node_names,
                        std::string label_node_name, std::string output_node_name,
                        DataType const &learning_rate)
  : graph_(std::move(graph))
  , input_node_names_(std::move(input_node_names))
  , label_node_name_(std::move(label_node_name))
  , output_node_name_(std::move(output_node_name))
  , learning_rate_(learning_rate)
  , epoch_(0)
{
  Init();
}

template <class T>
Optimiser<T>::Optimiser(std::shared_ptr<Graph<T>> graph, std::vector<std::string> input_node_names,
                        std::string label_node_name, std::string output_node_name,
                        LearningRateParam<DataType> learning_rate_param)
  : graph_(std::move(graph))
  , input_node_names_(std::move(input_node_names))
  , label_node_name_(std::move(label_node_name))
  , output_node_name_(std::move(output_node_name))
  , epoch_(0)
  , learning_rate_param_(std::move(learning_rate_param))
{
  // initialise learning rate
  learning_rate_ = learning_rate_param_.starting_learning_rate;

  Init();
}

/**
 * Does 1 training epoch using label array and vector of data arrays
 * @tparam T TensorType
 * @tparam C CriterionType
 * @param data training data
 * @param labels training labels
 * @param batch_size size of mini-batch, if batch_size==0 it will be set to n_data size
 * @return Sum of losses from all mini-batches
 */
template <class T>
typename T::Type Optimiser<T>::Run(std::vector<TensorType> const &data, TensorType const &labels,
                                   SizeType batch_size)
{
  assert(!data.empty());
  // Get trailing dimensions
  SizeType n_data_dimm = data.at(0).shape().size() - 1;
  SizeType n_data      = data.at(0).shape().at(n_data_dimm);

  // for some input combinations batch size will be modified
  batch_size = UpdateBatchSize(batch_size, n_data);
  loss_sum_  = 0;
  step_      = 0;
  loss_      = DataType{0};
  // variable for stats output
  start_time_ = std::chrono::high_resolution_clock::now();

  // Prepare output data tensors
  if (batch_data_.size() != data.size())
  {
    batch_data_.resize(data.size());
  }
  for (SizeType i{0}; i < data.size(); i++)
  {
    std::vector<SizeType> current_data_shape             = data.at(i).shape();
    current_data_shape.at(current_data_shape.size() - 1) = batch_size;
    if (batch_data_.at(i).shape() != current_data_shape)
    {
      batch_data_.at(i) = (TensorType{current_data_shape});
    }
  }
  // Prepare output label tensor
  std::vector<SizeType> labels_size           = labels.shape();
  SizeType              label_batch_dimension = labels_size.size() - 1;
  labels_size.at(label_batch_dimension)       = batch_size;
  if (batch_labels_.shape() != labels_size)
  {
    batch_labels_ = TensorType{labels_size};
  }
  SizeType k{0};
  while (step_ < n_data)
  {
    // Prepare batch
    SizeType it{step_};
    for (SizeType i{0}; i < batch_size; i++)
    {
      if (it >= n_data)
      {
        it = 0;
      }
      // Fill label view
      auto label_view = batch_labels_.View(i);
      label_view.Assign(labels.View(it));
      // Fill all data from data vector
      for (SizeType j{0}; j < data.size(); j++)
      {
        // Fill data[j] view
        auto data_view = batch_data_.at(j).View(i);
        data_view.Assign(data.at(j).View(it));
      }
      it++;
    }

    // Set inputs
    auto name_it = input_node_names_.begin();
    for (auto &input : batch_data_)
    {
      graph_->SetInputReference(*name_it, input);
      ++name_it;
    }

    // Set Label
    graph_->SetInputReference(label_node_name_, batch_labels_);

    auto loss_tensor = graph_->ForwardPropagate(output_node_name_);
    loss_ += *(loss_tensor.begin());
    graph_->BackPropagate(output_node_name_);

    // Compute and apply gradient
    ApplyGradients(batch_size);

    ResetGradients();

    step_ += batch_size;
    cumulative_step_ += batch_size;

    loss_sum_ += loss_;
    k++;
    loss_ = static_cast<DataType>(0);
    PrintStats(batch_size, n_data);

    UpdateLearningRate();
  }
  IncrementEpochCounter();

  return loss_sum_ / static_cast<DataType>(k);
}

/**
 * Does 1 training epoch using DataLoader
 * @tparam T TensorType
 * @tparam C CriterionType
 * @param loader DataLoader that provides examples for training
 * @param batch_size size of mini-batch
 * @param subset_size size of subset that will be loaded from DataLoader and trained in 1 epoch. If
 * not specified, epoch will end when DataLoader wouldn't have next example. Loader is being reset
 * on begining of each epoch.
 * @return Sum of losses from all mini-batches
 */
template <class T>
typename T::Type Optimiser<T>::Run(
    fetch::ml::dataloaders::DataLoader<TensorType, TensorType> &loader,
    LearningRateParam<DataType> learning_rate_param, SizeType batch_size, SizeType subset_size)
{
  // setting up learning_rate_param_
  learning_rate_param_ = learning_rate_param;

  // reset learning rate related parameters as learning schedule is reset
  cumulative_step_ = 0;
  epoch_           = 0;
  learning_rate_ - learning_rate_param_.starting_learning_rate;

  return RunImplementation(loader, batch_size, subset_size);
}

template <class T>
typename T::Type Optimiser<T>::Run(
    fetch::ml::dataloaders::DataLoader<TensorType, TensorType> &loader, SizeType batch_size,
    SizeType subset_size)
{
  return RunImplementation(loader, batch_size, subset_size);
}

template <class T>
typename T::Type Optimiser<T>::RunImplementation(
    fetch::ml::dataloaders::DataLoader<TensorType, TensorType> &loader, SizeType batch_size,
    SizeType subset_size)
{
  if (loader.IsDone())
  {
    loader.Reset();
  }

  // for some input combinations batch size will be modified
  batch_size = UpdateBatchSize(batch_size, loader.Size(), subset_size);
  loss_sum_  = 0;
  step_      = 0;
  loss_      = DataType{0};
  // variable for stats output
  start_time_ = std::chrono::high_resolution_clock::now();

  // tracks whether loader is done, but dataloader will reset inside Prepare batch
  bool is_done_set = loader.IsDone();

  std::pair<TensorType, std::vector<TensorType>> input;
  SizeType                                       i{0};

  // - check not completed more steps than user specified subset_size
  // - is_done_set checks if loader.IsDone inside PrepareBatch
  // - loader.IsDone handles edge case where batch divides perfectly into data set size
  while ((step_ < subset_size) && (!is_done_set) && (!loader.IsDone()))
  {
    is_done_set = false;

    // Do batch back-propagation
    input = loader.PrepareBatch(batch_size, is_done_set);

    auto name_it = input_node_names_.begin();
    for (auto &cur_input : input.second)
    {
      graph_->SetInputReference(*name_it, cur_input);
      ++name_it;
    }

    // Set Label
    graph_->SetInputReference(label_node_name_, input.first);

    auto loss_tensor = graph_->ForwardPropagate(output_node_name_);
    loss_ += *(loss_tensor.begin());
    graph_->BackPropagate(output_node_name_);

    // Compute and apply gradient
    ApplyGradients(batch_size);

    // reset graph gradients
    ResetGradients();

    // increment step
    step_ += batch_size;
    cumulative_step_ += batch_size;
    // reset loss
    loss_sum_ += loss_;
    i++;
    loss_ = DataType{0};
    // update learning rate
    UpdateLearningRate();

    // print the training stats every batch
    PrintStats(batch_size, subset_size);
  }

  epoch_++;
  return loss_sum_ / static_cast<DataType>(i);
}

template <typename T>
void Optimiser<T>::PrintStats(SizeType batch_size, SizeType subset_size)
{
  cur_time_  = std::chrono::high_resolution_clock::now();
  time_span_ = std::chrono::duration_cast<std::chrono::duration<double>>(cur_time_ - start_time_);
  if (subset_size == fetch::math::numeric_max<math::SizeType>())
  {
    stat_string_ =
        std::to_string(step_) + " (??%) -- " +
        "learning rate: " + std::to_string(static_cast<double>(learning_rate_)) + " -- " +
        std::to_string(static_cast<double>(step_) / static_cast<double>(time_span_.count())) +
        " samples / sec ";
  }
  else
  {
    stat_string_ =
        std::to_string(step_) + " / " + std::to_string(subset_size) + " (" +
        std::to_string(static_cast<SizeType>(100.0 * static_cast<double>(step_) /
                                             static_cast<double>(subset_size))) +
        "%) -- " + "learning rate: " + std::to_string(static_cast<double>(learning_rate_)) +
        " -- " +
        std::to_string(static_cast<double>(step_) / static_cast<double>(time_span_.count())) +
        " samples / sec ";
  }
  // print it in log
  FETCH_LOG_INFO("ML_LIB", "Training speed: ", stat_string_);
  // NOLINTNEXTLINE
  FETCH_LOG_INFO("ML_LIB", "Batch loss: ", loss_sum_ / static_cast<DataType>(step_ / batch_size));
}
/**
 *
 * @tparam T
 * @tparam C
 * @param new_learning_rate
 */
template <class T>
void Optimiser<T>::UpdateLearningRate()
{
  switch (learning_rate_param_.mode)
  {
  case LearningRateParam<DataType>::LearningRateDecay::EXPONENTIAL:
  {
    learning_rate_ = learning_rate_param_.starting_learning_rate *
                     fetch::math::Pow(learning_rate_param_.exponential_decay_rate,
                                      static_cast<DataType>(epoch_ + 1));
    break;
  }
  case LearningRateParam<DataType>::LearningRateDecay::LINEAR:
  {
    learning_rate_ = learning_rate_param_.starting_learning_rate *
                     (static_cast<DataType>(1) - learning_rate_param_.linear_decay_rate *
                                                     static_cast<DataType>(cumulative_step_));
    if (learning_rate_ < learning_rate_param_.ending_learning_rate)
    {
      learning_rate_ = learning_rate_param_.ending_learning_rate;
    }
    break;
  }
  case LearningRateParam<DataType>::LearningRateDecay::NONE:
  {
    break;
  }
  default:
  {
    throw exceptions::InvalidMode("Please specify learning rate schedule method");
  }
  }
}

/**
 * Increments epoch counter
 * @tparam T
 */
template <class T>
void Optimiser<T>::IncrementEpochCounter()
{
  epoch_++;
}

/**
 * Increments batch counters
 * @tparam T
 */
template <class T>
void Optimiser<T>::IncrementBatchCounters(SizeType batch_size)
{
  step_ += batch_size;
  cumulative_step_ += batch_size;
}

/**
 * helper method for setting or updating batch size in case of invalid parameter combinations.
 * For example, batch_size cannot be larger than data_size or subset_size.
 * @tparam T
 * @tparam C
 * @param batch_size
 * @param data_size
 * @param subset_size
 * @return
 */
template <class T>
typename Optimiser<T>::SizeType Optimiser<T>::UpdateBatchSize(SizeType const &batch_size,
                                                              SizeType const &data_size,
                                                              SizeType const &subset_size)
{
  SizeType updated_batch_size = batch_size;
  // If batch_size not specified do full batch
  if (batch_size == SIZE_NOT_SET)
  {
    if (subset_size == SIZE_NOT_SET)
    {
      updated_batch_size = data_size;
    }
    else
    {
      updated_batch_size = subset_size;
    }
  }
  // If user sets smaller subset size than batch size
  if (batch_size > subset_size)
  {
    updated_batch_size = subset_size;
  }
  // if batch size is larger than all data
  if (batch_size > data_size)
  {
    updated_batch_size = data_size;
  }
  return updated_batch_size;
}

template <typename T>
void Optimiser<T>::ResetGradients()
{
  this->graph_->ResetGradients();
}

template <typename T>
std::shared_ptr<Graph<T>> Optimiser<T>::GetGraph()
{
  return graph_;
}

}  // namespace optimisers
}  // namespace ml

namespace serializers {
/**
 * serializer for Optimiser
 * @tparam TensorType
 */
template <typename TensorType, typename D>
struct MapSerializer<ml::optimisers::Optimiser<TensorType>, D>
{
  using Type       = ml::optimisers::Optimiser<TensorType>;
  using DriverType = D;

  // public member variables
  static uint8_t const GRAPH               = 1;
  static uint8_t const INPUT_NODE_NAMES    = 2;
  static uint8_t const LABEL_NODE_NAME     = 3;
  static uint8_t const OUTPUT_NODE_NAME    = 4;
  static uint8_t const LEARNING_RATE       = 5;
  static uint8_t const LEARNING_RATE_PARAM = 6;
  static uint8_t const EPOCH               = 7;

  // private member variables
  static uint8_t const LOSS            = 8;
  static uint8_t const LOSS_SUM        = 9;
  static uint8_t const STEP            = 10;
  static uint8_t const CUMULATIVE_STEP = 11;
  static uint8_t const INPUT_FIRST     = 12;
  static uint8_t const INPUT_SECOND    = 13;
  static uint8_t const CUR_LABEL       = 14;
  static uint8_t const PRED_LABEL      = 15;
  static uint8_t const CUR_TIME        = 16;
  static uint8_t const START_TIME      = 17;
  static uint8_t const TIME_SPAN       = 18;
  static uint8_t const STAT_STRING     = 19;
  static uint8_t const BATCH_DATA      = 20;
  static uint8_t const BATCH_LABELS    = 21;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &sp)
  {
    auto map = map_constructor(21);

    // serialize the graph first
    map.Append(GRAPH, sp.graph_->GetGraphSaveableParams());

    map.Append(INPUT_NODE_NAMES, sp.input_node_names_);
    map.Append(LABEL_NODE_NAME, sp.label_node_name_);
    map.Append(OUTPUT_NODE_NAME, sp.output_node_name_);
    map.Append(LEARNING_RATE, sp.learning_rate_);
    map.Append(LEARNING_RATE_PARAM, sp.learning_rate_param_);

    map.Append(EPOCH, sp.epoch_);
    map.Append(LOSS, sp.loss_);
    map.Append(LOSS_SUM, sp.loss_sum_);
    map.Append(STEP, sp.step_);
    map.Append(CUMULATIVE_STEP, sp.cumulative_step_);

    map.Append(INPUT_FIRST, sp.input_.first);
    map.Append(INPUT_SECOND, sp.input_.second);

    map.Append(CUR_LABEL, sp.cur_label_);
    map.Append(PRED_LABEL, sp.pred_label_);

    map.Append(STAT_STRING, sp.stat_string_);
    map.Append(BATCH_DATA, sp.batch_data_);
    map.Append(BATCH_LABELS, sp.batch_labels_);
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &sp)
  {
    // deserialize the graph first
    fetch::ml::GraphSaveableParams<TensorType> gsp;
    map.ExpectKeyGetValue(GRAPH, gsp);
    auto graph_ptr = std::make_shared<fetch::ml::Graph<TensorType>>();
    ml::utilities::BuildGraph(gsp, graph_ptr);
    sp.graph_ = graph_ptr;

    map.ExpectKeyGetValue(INPUT_NODE_NAMES, sp.input_node_names_);
    map.ExpectKeyGetValue(LABEL_NODE_NAME, sp.label_node_name_);
    map.ExpectKeyGetValue(OUTPUT_NODE_NAME, sp.output_node_name_);
    map.ExpectKeyGetValue(LEARNING_RATE, sp.learning_rate_);
    map.ExpectKeyGetValue(LEARNING_RATE_PARAM, sp.learning_rate_param_);

    // recover gradients and gradient trainables from graph
    sp.Init();

    map.ExpectKeyGetValue(EPOCH, sp.epoch_);
    map.ExpectKeyGetValue(LOSS, sp.loss_);
    map.ExpectKeyGetValue(LOSS_SUM, sp.loss_sum_);
    map.ExpectKeyGetValue(STEP, sp.step_);
    map.ExpectKeyGetValue(CUMULATIVE_STEP, sp.cumulative_step_);

    map.ExpectKeyGetValue(INPUT_FIRST, sp.input_.first);
    map.ExpectKeyGetValue(INPUT_SECOND, sp.input_.second);

    map.ExpectKeyGetValue(CUR_LABEL, sp.cur_label_);
    map.ExpectKeyGetValue(PRED_LABEL, sp.pred_label_);

    map.ExpectKeyGetValue(STAT_STRING, sp.stat_string_);
    map.ExpectKeyGetValue(BATCH_DATA, sp.batch_data_);
    map.ExpectKeyGetValue(BATCH_LABELS, sp.batch_labels_);
  }
};
}  // namespace serializers

}  // namespace fetch
