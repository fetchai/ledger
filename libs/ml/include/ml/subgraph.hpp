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

template <class T>
class SubGraph : public Graph<T>, public Ops<T>
{
public:
  using ArrayType    = T;
  using ArrayPtrType = std::shared_ptr<ArrayType>;

  virtual ArrayPtrType Forward(std::vector<ArrayPtrType> const &inputs)
  {
    ASSERT(inputs.size() == this->inputs_nodes_.size());
    for (size_t i(0); i < inputs.size(); ++i)
    {
      this->SetInput(inputs_nodes_[i], inputs[i]);
    }
    this->output_ = output_node_->Evaluate();
    return this->output_;
  }

  virtual std::vector<ArrayPtrType> Backward(std::vector<ArrayPtrType> const &inputs,
                                             ArrayPtrType                     error)
  {
    ASSERT(inputs.size() == this->inputs_nodes_.size());
    std::vector<std::pair<NodeInterface<T> *, ArrayPtrType>> nonBackpropagatedErrorSignals =
        this->output_node_->BackPropagate(error);
    std::vector<ArrayPtrType> backpropagatedErrorSignals;

    for (std::string const &s : inputs_nodes_)
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
  void AddInputNodes(std::string const &nodeName)
  {
    inputs_nodes_.push_back(nodeName);
  }

  void SetOutputNode(std::string const &nodeName)
  {
    output_node_ = this->nodes_[nodeName];
  }

protected:
  SubGraph()
  {}

private:
  std::vector<std::string>          inputs_nodes_;
  std::shared_ptr<NodeInterface<T>> output_node_;
};

}  // namespace ml
}  // namespace fetch
