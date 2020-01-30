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

namespace fetch {
namespace ml {

template <typename TensorType>
struct SubGraphSaveableParams;

/**
 * A SubGraph is a collection of nodes in the graph.
 * Layers should inherit from SubGraph
 * @tparam T  the tensor/array type
 */
template <class T>
class SubGraph : public Graph<T>, public ops::Ops<T>
{
public:
  using TensorType       = T;
  using VecTensorType    = std::vector<std::shared_ptr<TensorType const>>;
  using SPType           = SubGraphSaveableParams<TensorType>;
  using OpPtrType        = std::shared_ptr<fetch::ml::ops::Ops<TensorType>>;
  using NodePtrType      = std::shared_ptr<Node<TensorType>>;
  using NodeErrorMapType = std::unordered_map<Node<TensorType> *, std::vector<TensorType>>;

  static constexpr char const *DESCRIPTOR = "SubGraph";

  void                    Forward(VecTensorType const &inputs, TensorType &output) override;
  std::vector<TensorType> Backward(VecTensorType const &inputs,
                                   TensorType const &   error_signal) override;
  void                    AddInputNode(std::string const &node_name);
  void                    SetOutputNode(std::string const &node_name);

  std::shared_ptr<OpsSaveableParams>               GetOpSaveableParams() override;
  std::shared_ptr<fetch::ml::ops::Ops<TensorType>> MakeSharedCopy(
      std::shared_ptr<fetch::ml::ops::Ops<TensorType>> me) override;

protected:
  void InsertSharedCopy(std::shared_ptr<fetch::ml::ops::Ops<TensorType>> output_ptr);
  SubGraph() = default;
  std::vector<std::string> input_node_names_;
  std::string              output_node_name_;
};

}  // namespace ml
}  // namespace fetch
