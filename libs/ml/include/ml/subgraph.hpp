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

#include <cassert>
#include <memory>
#include <string>
#include <utility>
#include <vector>

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
  using ArrayType     = T;
  using VecTensorType = std::vector<std::reference_wrapper<ArrayType const>>;

  virtual void                   Forward(VecTensorType const &inputs, ArrayType &output);
  virtual std::vector<ArrayType> Backward(VecTensorType const &inputs,
                                          ArrayType const &    error_signal);

protected:
  void AddInputNode(std::string const &node_name);
  void SetOutputNode(std::string const &node_name);

protected:
  SubGraph() = default;

private:
  std::vector<std::string>          input_nodes_;
  std::shared_ptr<NodeInterface<T>> output_node_;
};

/**
 *
 * @tparam T
 * @param inputs
 * @param output
 * @return
 */
template <typename T>
void SubGraph<T>::Forward(VecTensorType const &inputs, ArrayType &output)
{
  assert(inputs.size() == this->input_nodes_.size());
  for (uint64_t i(0); i < inputs.size(); ++i)
  {
    this->SetInput(input_nodes_[i], inputs.at(i));
  }
  output = output_node_->Evaluate(this->is_training_);
}

template <typename T>
std::vector<T> SubGraph<T>::Backward(VecTensorType const &inputs, ArrayType const &error_signal)
{
  assert(inputs.size() == this->input_nodes_.size());
  FETCH_UNUSED(inputs);
  std::vector<std::pair<NodeInterface<T> *, ArrayType>> non_back_prop_err_signal =
      this->output_node_->BackPropagateSignal(error_signal);
  std::vector<ArrayType> back_prop_err_signal;

  for (std::string const &s : input_nodes_)
  {
    std::shared_ptr<NodeInterface<T>> node = this->nodes_[s];
    for (auto const &grad : non_back_prop_err_signal)
    {
      if (grad.first == node.get())
      {
        back_prop_err_signal.emplace_back(grad.second);
      }
    }
  }
  return back_prop_err_signal;
}

template <typename T>
void SubGraph<T>::AddInputNode(std::string const &node_name)
{
  input_nodes_.push_back(node_name);
}

template <typename T>
void SubGraph<T>::SetOutputNode(std::string const &node_name)
{
  output_node_ = this->nodes_[node_name];
}

}  // namespace ml
}  // namespace fetch
