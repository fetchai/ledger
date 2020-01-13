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

#include "ml/core/graph.hpp"
//#include "ml/ops/metrics/categorical_accuracy.hpp"

//#include "ml/layers/PRelu.hpp"
//#include "ml/layers/convolution_1d.hpp"
//#include "ml/layers/convolution_2d.hpp"
//#include "ml/layers/fully_connected.hpp"
//#include "ml/layers/multihead_attention.hpp"
//#include "ml/layers/scaled_dot_product_attention.hpp"
//#include "ml/layers/self_attention_encoder.hpp"
//#include "ml/layers/skip_gram.hpp"

#include "ml/meta/ml_type_traits.hpp"

//#include "ml/ops/abs.hpp"
//#include "ml/ops/activations/dropout.hpp"
//#include "ml/ops/activations/elu.hpp"
//#include "ml/ops/activations/gelu.hpp"
//#include "ml/ops/activations/leaky_relu.hpp"
//#include "ml/ops/activations/logsigmoid.hpp"
//#include "ml/ops/activations/logsoftmax.hpp"
//#include "ml/ops/activations/randomised_relu.hpp"
//#include "ml/ops/activations/relu.hpp"
//#include "ml/ops/activations/sigmoid.hpp"
//#include "ml/ops/activations/softmax.hpp"
//#include "ml/ops/add.hpp"
//#include "ml/ops/avg_pool_1d.hpp"
//#include "ml/ops/avg_pool_2d.hpp"
//#include "ml/ops/concatenate.hpp"
//#include "ml/ops/convolution_1d.hpp"
//#include "ml/ops/convolution_2d.hpp"
//#include "ml/ops/divide.hpp"
//#include "ml/ops/embeddings.hpp"
//#include "ml/ops/exp.hpp"
//#include "ml/ops/flatten.hpp"
//#include "ml/ops/log.hpp"
//#include "ml/ops/loss_functions/cross_entropy_loss.hpp"
//#include "ml/ops/loss_functions/mean_square_error_loss.hpp"
//#include "ml/ops/loss_functions/softmax_cross_entropy_loss.hpp"
//#include "ml/ops/mask_fill.hpp"
//#include "ml/ops/matrix_multiply.hpp"
//#include "ml/ops/max_pool.hpp"
//#include "ml/ops/max_pool_1d.hpp"
//#include "ml/ops/max_pool_2d.hpp"
//#include "ml/ops/maximum.hpp"
//#include "ml/ops/multiply.hpp"
//#include "ml/ops/one_hot.hpp"
//#include "ml/ops/placeholder.hpp"
//#include "ml/ops/reduce_mean.hpp"
//#include "ml/ops/reshape.hpp"
//#include "ml/ops/slice.hpp"
//#include "ml/ops/sqrt.hpp"
//#include "ml/ops/squeeze.hpp"
//#include "ml/ops/strided_slice.hpp"
//#include "ml/ops/subtract.hpp"
//#include "ml/ops/switch.hpp"
//#include "ml/ops/tanh.hpp"
//#include "ml/ops/top_k.hpp"
//#include "ml/ops/transpose.hpp"
//#include "ml/ops/weights.hpp"

#include "ml/core/subgraph.hpp"

#include "ml/saveparams/saveable_params.hpp"

namespace fetch {
namespace ml {
namespace utilities {

template <typename T>
void BuildNodeAndInsertTrainables(NodeSaveableParams<T> const &nsp, std::string const &name,
                                  std::shared_ptr<Graph<T>> g);

template <typename T>
void BuildGraph(GraphSaveableParams<T> const &sp, std::shared_ptr<Graph<T>> ret)
{
  // loop through connections adding nodes
  for (auto &node : sp.nodes)
  {
    std::string node_name = node.first;

    // Check if graph has shared weights
    std::string suffix = "_Copy_1";
    if (node_name.size() >= suffix.size() &&
        node_name.compare(node_name.size() - suffix.size(), suffix.size(), suffix) == 0)
    {
      throw ml::exceptions::NotImplemented("Cannot currently deserialize shared-weights graph");
    }

    BuildNodeAndInsertTrainables(*(std::dynamic_pointer_cast<NodeSaveableParams<T>>(node.second)),
                                 node_name, ret);
  }
  ret->SetGraphSaveableParams(sp);
}

template <typename T>
void BuildSubGraph(SubGraphSaveableParams<T> const &sgsp, std::shared_ptr<SubGraph<T>> ret)
{
  BuildGraph<T>(sgsp, ret);

  for (auto const &name : sgsp.input_node_names)
  {
    ret->AddInputNode(name);
  }
  ret->SetOutputNode(sgsp.output_node_name);
}

template <typename T, typename OperationType>
std::shared_ptr<OperationType> BuildLayer(std::shared_ptr<OpsSaveableParams> op_save_params)
{
  auto sp  = *(std::dynamic_pointer_cast<typename OperationType::SPType>(op_save_params));
  auto ret = std::make_shared<OperationType>();
  BuildSubGraph<T>(sp, ret);
  ret->SetOpSaveableParams(sp);
  return ret;
}

template <class OperationType>
std::shared_ptr<OperationType> GetOp(std::shared_ptr<OpsSaveableParams> op_save_params)
{
  return std::make_shared<OperationType>(
      *(std::dynamic_pointer_cast<typename OperationType::SPType>(op_save_params)));
}

}  // namespace utilities
}  // namespace ml
}  // namespace fetch
