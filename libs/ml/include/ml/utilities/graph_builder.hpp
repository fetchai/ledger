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
#include "ml/core/subgraph.hpp"
#include "ml/meta/ml_type_traits.hpp"

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
