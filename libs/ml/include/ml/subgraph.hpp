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

#include "ml/graph.hpp"
#include <iostream>
#include <memory>

namespace fetch {
namespace ml {

/**
 * A SubGraph is a collection of nodes in the graph.
 * Layers should inherit from SubGraph
 * @tparam T  the tensor/array type
 */
template <class T>
class SubGraph : public Graph<T>, public Ops<T>
{
public:
  using ArrayType    = T;
  using ArrayPtrType = std::shared_ptr<ArrayType>;

  virtual ArrayType Forward(std::vector<std::reference_wrapper<ArrayType const>> const &inputs)
  {
    ASSERT(inputs.size() == this->input_nodes_.size());
    for (std::uint64_t i(0); i < inputs.size(); ++i)
    {
      this->SetInput(input_nodes_[i], inputs.at(i));
    }
    return output_node_->Evaluate();
  }

  virtual std::vector<ArrayType> Backward(
      std::vector<std::reference_wrapper<ArrayType const>> const &inputs,
      ArrayType const &                                           errorSignal)
  {
    ASSERT(inputs.size() == this->input_nodes_.size());
    std::vector<std::pair<NodeInterface<T> *, ArrayType>> nonBackpropagatedErrorSignals =
        this->output_node_->BackPropagate(errorSignal);
    std::vector<ArrayType> backpropagatedErrorSignals;

    for (std::string const &s : input_nodes_)
    {
      std::shared_ptr<NodeInterface<T>> node = this->nodes_[s];
      for (auto const &grad : nonBackpropagatedErrorSignals)
      {
        if (grad.first == node.get())
        {
          backpropagatedErrorSignals.push_back(grad.second);
        }
      }
    }
    return backpropagatedErrorSignals;
  }

protected:
  void AddInputNodes(std::string const &node_name)
  {
    input_nodes_.push_back(node_name);
  }
  void AddInputNodes(std::vector<std::string> const &node_names)
  {
    input_nodes_ = node_names;
  }

  void SetOutputNode(std::string const &node_name)
  {
    output_node_ = this->nodes_[node_name];
  }

protected:
  SubGraph() = default;

private:
  std::vector<std::string>          input_nodes_;
  std::shared_ptr<NodeInterface<T>> output_node_;
};

}  // namespace ml
}  // namespace fetch
