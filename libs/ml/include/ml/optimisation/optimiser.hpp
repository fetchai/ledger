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
#include "ml/dataloaders/dataloader.hpp"
#include "ml/graph.hpp"
#include "ml/ops/loss_functions/criterion.hpp"

namespace fetch {
namespace ml {
namespace optimisers {

static constexpr fetch::math::SizeType SIZE_NOT_SET = fetch::math::numeric_max<math::SizeType>();

/**
 * Abstract gradient optimiser class
 * @tparam T ArrayType
 * @tparam C CriterionType
 */
template <class T, class C>
class Optimiser
{
public:
  using ArrayType     = T;
  using CriterionType = C;
  using DataType      = typename ArrayType::Type;
  using SizeType      = typename ArrayType::SizeType;

  Optimiser(std::shared_ptr<Graph<T>> graph, std::vector<std::string> input_node_names,
            std::string output_node_name, DataType const &learning_rate = DataType(0.001),
            DataType const &delta_learning_rate = DataType(1.0));

  virtual ~Optimiser() = default;

  // TODO (private 1090): Optimise TensorSlice for graph-feeding without using .Copy
  DataType Run(std::vector<ArrayType> const &data, ArrayType const &labels,
               SizeType batch_size = SIZE_NOT_SET);

  DataType Run(fetch::ml::dataloaders::DataLoader<ArrayType, ArrayType> &loader,
               SizeType batch_size = SIZE_NOT_SET, SizeType subset_size = SIZE_NOT_SET);

  void     UpdateLearningRate();
  SizeType UpdateBatchSize(SizeType const &batch_size, SizeType const &data_size,
                           SizeType const &subset_size = SIZE_NOT_SET);

protected:
  std::shared_ptr<Graph<T>> graph_;
  CriterionType             criterion_;
  std::vector<std::string>  input_node_names_ = {};
  std::string               output_node_name_ = "";

  DataType learning_rate_       = fetch::math::numeric_max<DataType>();
  DataType delta_learning_rate_ = fetch::math::numeric_max<DataType>();

  std::vector<std::shared_ptr<fetch::ml::ops::Trainable<ArrayType>>> graph_trainables_;
  std::vector<ArrayType>                                             gradients_;
  SizeType                                                           epoch_ = SIZE_NOT_SET;

private:
  std::vector<ArrayType> batch_data_;
  ArrayType              batch_labels_;

  virtual void ApplyGradients(SizeType batch_size) = 0;
};

template <class T, class C>
Optimiser<T, C>::Optimiser(std::shared_ptr<Graph<T>> graph,
                           std::vector<std::string> input_node_names, std::string output_node_name,
                           DataType const &learning_rate, DataType const &delta_learning_rate)
  : graph_(graph)
  , input_node_names_(std::move(input_node_names))
  , output_node_name_(std::move(output_node_name))
  , learning_rate_(learning_rate)
  , delta_learning_rate_(delta_learning_rate)
  , epoch_(0)
{
  graph_trainables_ = graph_->get_trainables();
  for (auto &train : graph_trainables_)
  {
    this->gradients_.emplace_back(ArrayType(train->get_weights().shape()));
  }
}

/**
 * Does 1 training epoch using label array and vector of data arrays
 * @tparam T ArrayType
 * @tparam C CriterionType
 * @param data training data
 * @param labels training labels
 * @param batch_size size of mini-batch, if batch_size==0 it will be set to n_data size
 * @return Sum of losses from all mini-batches
 */
// TODO (private 1090): Optimise TensorSlice for graph-feeding without using .Copy
template <class T, class C>
typename T::Type Optimiser<T, C>::Run(std::vector<ArrayType> const &data, ArrayType const &labels,
                                      SizeType batch_size)
{
  assert(data.size() > 0);

  // Get trailing dimensions
  SizeType n_data_dimm = data.at(0).shape().size() - 1;
  SizeType n_data      = data.at(0).shape().at(n_data_dimm);

  // for some input combinations batch size will be modified
  batch_size = UpdateBatchSize(batch_size, n_data);

  DataType loss{0};
  DataType loss_sum{0};

  SizeType step{0};

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
      batch_data_.at(i) = (ArrayType{current_data_shape});
    }
  }

  // Prepare output label tensor
  std::vector<SizeType> labels_size           = labels.shape();
  SizeType              label_batch_dimension = labels_size.size() - 1;
  labels_size.at(label_batch_dimension)       = batch_size;
  if (batch_labels_.shape() != labels_size)
  {
    batch_labels_ = ArrayType{labels_size};
  }

  while (step < n_data)
  {
    // Prepare batch
    SizeType it{step};
    for (SizeType i{0}; i < batch_size; i++)
    {
      if (it >= n_data)
      {
        it = 0;
      }

      // Fill label slice
      auto label_slice = batch_labels_.Slice(i, label_batch_dimension);
      label_slice.Assign(labels.Slice(it, label_batch_dimension));

      // Fill all data from data vector
      for (SizeType j{0}; j < data.size(); j++)
      {
        // Fill data[j] slice
        SizeType cur_data_batch_dim = data.at(j).shape().size() - 1;
        auto     data_slice         = batch_data_.at(j).Slice(i, cur_data_batch_dim);
        data_slice.Assign(data.at(j).Slice(it, cur_data_batch_dim));
      }
      it++;
    }

    auto name_it = input_node_names_.begin();
    for (auto &input : batch_data_)
    {
      graph_->SetInput(*name_it, input);
      ++name_it;
    }

    auto label_pred = graph_->Evaluate(output_node_name_);
    loss            = criterion_.Forward({label_pred, batch_labels_});
    graph_->BackPropagate(output_node_name_, criterion_.Backward({label_pred, batch_labels_}));

    // Compute and apply gradient
    ApplyGradients(batch_size);

    //    FETCH_LOG_INFO("ML_LIB", "Batch loss: ", loss);

    step += batch_size;
    loss_sum += loss;
  }

  UpdateLearningRate();
  epoch_++;

  return loss_sum;
}

/**
 * Does 1 training epoch using DataLoader
 * @tparam T ArrayType
 * @tparam C CriterionType
 * @param loader DataLoader that provides examples for training
 * @param batch_size size of mini-batch
 * @param subset_size size of subset that will be loaded from DataLoader and trained in 1 epoch. If
 * not specified, epoch will end when DataLoader wouldn't have next example. Loader is being reset
 * on begining of each epoch.
 * @return Sum of losses from all mini-batches
 */
template <class T, class C>
typename T::Type Optimiser<T, C>::Run(
    fetch::ml::dataloaders::DataLoader<ArrayType, ArrayType> &loader, SizeType batch_size,
    SizeType subset_size)
{
  if (loader.IsDone())
  {
    loader.Reset();
  }

  // for some input combinations batch size will be modified
  batch_size = UpdateBatchSize(batch_size, loader.Size(), subset_size);

  DataType loss{0};
  DataType loss_sum{0};
  SizeType step{0};

  // tracks whether loader is done, but dataloader will reset inside Prepare batch
  bool is_done_set = !loader.IsDone();

  std::pair<ArrayType, std::vector<ArrayType>> input;
  while ((step < subset_size) && (!is_done_set))
  {
    is_done_set = false;
    loss        = DataType{0};

    // Do batch back-propagation
    input = loader.PrepareBatch(batch_size, is_done_set);

    auto cur_input = input.second;

    auto name_it = input_node_names_.begin();
    for (auto &cur_input : input.second)
    {
      graph_->SetInput(*name_it, cur_input);
      ++name_it;
    }

    auto cur_label  = input.first;
    auto label_pred = graph_->Evaluate(output_node_name_);
    loss            = criterion_.Forward({label_pred, cur_label});
    graph_->BackPropagate(output_node_name_, criterion_.Backward({label_pred, cur_label}));

    // Compute and apply gradient
    ApplyGradients(batch_size);

    //    FETCH_LOG_INFO("ML_LIB", "Batch loss: ", loss);

    step += batch_size;
    loss_sum += loss;
  }

  UpdateLearningRate();
  epoch_++;

  return loss_sum;
}

/**
 *
 * @tparam T
 * @tparam C
 * @param new_learning_rate
 */
template <class T, class C>
void Optimiser<T, C>::UpdateLearningRate()
{
  learning_rate_ = learning_rate_ * delta_learning_rate_;
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
template <class T, class C>
typename Optimiser<T, C>::SizeType Optimiser<T, C>::UpdateBatchSize(SizeType const &batch_size,
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

}  // namespace optimisers
}  // namespace ml
}  // namespace fetch
