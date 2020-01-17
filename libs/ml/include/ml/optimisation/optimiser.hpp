#pragma once
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

#include "math/tensor/tensor.hpp"
#include "ml/dataloaders/dataloader.hpp"
#include "ml/optimisation/learning_rate_params.hpp"


namespace fetch {
namespace ml {


template <class T>
class Graph;

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
  using SizeType   = fetch::math::SizeType;

  Optimiser() = default;
  Optimiser(std::shared_ptr<Graph<T>> graph, std::vector<std::string> input_node_names,
            std::string label_node_name, std::string output_node_name,
            DataType const &learning_rate = fetch::math::Type<DataType>("0.001"));

  Optimiser(std::shared_ptr<Graph<T>> graph, std::vector<std::string> input_node_names,
            std::string label_node_name, std::string output_node_name,
            LearningRateParam<DataType> learning_rate_param);

  virtual ~Optimiser() = default;

  inline void SetGraph(std::shared_ptr<Graph<T>> graph)
  {
    graph_ = graph;
  }

  /// DATA RUN INTERFACES ///
  DataType Run(std::vector<TensorType> const &data, TensorType const &labels,
               SizeType batch_size = SIZE_NOT_SET);

  /// DATALOADER RUN INTERFACES ///
  DataType Run(fetch::ml::dataloaders::DataLoader<TensorType> &loader,
               SizeType batch_size = SIZE_NOT_SET, SizeType subset_size = SIZE_NOT_SET);
  DataType Run(fetch::ml::dataloaders::DataLoader<TensorType> &loader,
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
  virtual OptimiserType OptimiserCode() = 0;

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

  DataType RunImplementation(fetch::ml::dataloaders::DataLoader<TensorType> &loader,
                             SizeType batch_size  = SIZE_NOT_SET,
                             SizeType subset_size = SIZE_NOT_SET);
};

}  // namespace optimisers
}  // namespace ml
}  // namespace fetch
