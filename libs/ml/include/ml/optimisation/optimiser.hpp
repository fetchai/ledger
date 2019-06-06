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
#include "ml/graph.hpp"
#include "ml/ops/loss_functions/criterion.hpp"

namespace fetch {
namespace ml {
namespace optimisers {

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

  Optimiser(std::shared_ptr<Graph<T>>

                               graph,
            std::string const &input_node_name, std::string const &output_node_name,
            DataType const &learning_rate = DataType{0.001f});

  // TODO (private 1090): Optimise TensorSlice for graph-feeding without using .Copy
  DataType Run(ArrayType &data, ArrayType &labels, SizeType batch_size = 0);

protected:
  std::shared_ptr<Graph<T>> graph_;
  CriterionType             criterion_;
  std::string               input_node_name_  = "";
  std::string               output_node_name_ = "";
  DataType                  learning_rate_    = fetch::math::numeric_max<DataType>();
  std::vector<std::shared_ptr<fetch::ml::ops::Trainable<ArrayType>>> graph_trainables_;
  std::vector<ArrayType>                                             gradients_;
  SizeType epoch_ = fetch::math::numeric_max<SizeType>();

private:
  virtual void ApplyGradients(SizeType batch_size) = 0;
};

template <class T, class C>
Optimiser<T, C>::Optimiser(std::shared_ptr<Graph<T>> graph, std::string const &input_node_name,
                           std::string const &output_node_name, DataType const &learning_rate)
  :

  graph_(graph)
  , input_node_name_(input_node_name)
  , output_node_name_(output_node_name)
  , learning_rate_(learning_rate)
  , epoch_(0)
{
  graph_trainables_ = graph_->get_trainables();
  for (auto &train : graph_trainables_)
  {
    this->gradients_.emplace_back(ArrayType(train->get_weights().shape()));
  }
}

/**
 *
 * @tparam T ArrayType
 * @tparam C CriterionType
 * @param data training data
 * @param labels training labels
 * @param batch_size size of mini-batch, if batch_size==0 it will be set to n_data size
 * @return Sum of losses from all mini-batches
 */
// TODO (private 1090): Optimise TensorSlice for graph-feeding without using .Copy
template <class T, class C>
typename T::Type Optimiser<T, C>::Run(ArrayType &data, ArrayType &labels, SizeType batch_size)
{
  // Get trailing dimensions
  SizeType n_data_dimm  = data.shape().size() - 1;
  SizeType n_label_dimm = labels.shape().size() - 1;

  SizeType n_data = data.shape().at(n_data_dimm);

  // If batch_size is not specified do full batch
  if (batch_size == 0)
  {
    batch_size = n_data;
  }

  DataType loss{0};
  SizeType step{0};
  while (step < n_data)
  {
    loss = DataType{0};

    // Do batch back-propagation
    for (SizeType it{step}; (it < step + batch_size) && (it < n_data); ++it)
    {
      auto cur_input = data.Slice(it, n_data_dimm).Copy();
      graph_->SetInput(input_node_name_, cur_input);
      auto cur_label  = labels.Slice(it, n_label_dimm).Copy();
      auto label_pred = graph_->Evaluate(output_node_name_);
      loss += criterion_.Forward({label_pred, cur_label});
      graph_->BackPropagate(output_node_name_, criterion_.Backward({label_pred, cur_label}));
    }
    // Compute and apply gradient
    ApplyGradients(batch_size);

    FETCH_LOG_INFO("ML_LIB", "Loss: ", loss);

    step += batch_size;
  }

  DataType loss_sum{0};
  loss_sum += loss;
  epoch_++;

  return loss_sum;
}

}  // namespace optimisers
}  // namespace ml
}  // namespace fetch
