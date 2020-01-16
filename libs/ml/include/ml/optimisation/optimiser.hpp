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

#include "math/base_types.hpp"
#include "math/standard_functions/pow.hpp"
#include "math/statistics/mean.hpp"
#include "ml/core/graph.hpp"
#include "ml/dataloaders/dataloader.hpp"
#include "ml/meta/ml_type_traits.hpp"
#include "ml/optimisation/learning_rate_params.hpp"
#include "ml/utilities/graph_builder.hpp"

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
  using SizeType   = fetch::math::SizeType;

  Optimiser() = default;
  Optimiser(std::shared_ptr<Graph<T>> graph, std::vector<std::string> input_node_names,
            std::string label_node_name, std::string output_node_name,
            DataType const &learning_rate = fetch::math::Type<DataType>("0.001"));

  Optimiser(std::shared_ptr<Graph<T>> graph, std::vector<std::string> input_node_names,
            std::string label_node_name, std::string output_node_name,
            LearningRateParam<DataType> learning_rate_param);

  virtual ~Optimiser() = default;

  void SetGraph(std::shared_ptr<Graph<T>> graph)
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
