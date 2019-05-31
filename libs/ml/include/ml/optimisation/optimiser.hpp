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
  DataType DoBatch(ArrayType &data, ArrayType &labels);

protected:
  std::shared_ptr<Graph<T>> graph_;
  CriterionType             criterion_;
  std::string               input_node_name_  = "";
  std::string               output_node_name_ = "";
  DataType                  learning_rate_    = fetch::math::numeric_max<DataType>();
  std::vector<std::shared_ptr<fetch::ml::ops::Trainable<ArrayType>>> graph_trainables_;
  std::vector<ArrayType>                                             gradients_;

private:
  virtual void ApplyGradients() = 0;
};

template <class T, class C>
Optimiser<T, C>::Optimiser(std::shared_ptr<Graph<T>> graph, std::string const &input_node_name,
                           std::string const &output_node_name, DataType const &learning_rate)
  :

  graph_(graph)
  , input_node_name_(input_node_name)
  , output_node_name_(output_node_name)
  , learning_rate_(learning_rate)
{
  graph_trainables_ = graph_->GetTrainables();

  for (auto &train : graph_trainables_)
  {
    this->gradients_.emplace_back(ArrayType(train->GetWeights().shape()));
  }
}

// TODO (private 1090): Optimise TensorSlice for graph-feeding without using .Copy
template <class T, class C>
typename T::Type Optimiser<T, C>::DoBatch(ArrayType &data, ArrayType &labels)
{
  DataType loss{0};
  SizeType n_data = data.shape().at(0);

  // Do batch back-propagation
  for (SizeType step{0}; step < n_data; ++step)
  {
    auto cur_input = data.Slice(step).Copy();
    graph_->SetInput(input_node_name_, cur_input);
    auto cur_label  = labels.Slice(step).Copy();
    auto label_pred = graph_->Evaluate(output_node_name_);
    loss += criterion_.Forward({label_pred, cur_label});
    graph_->BackPropagate(output_node_name_, criterion_.Backward({label_pred, cur_label}));
  }

  // Compute and apply gradient
  ApplyGradients();

  return loss;
}

}  // namespace optimisers
}  // namespace ml
}  // namespace fetch
