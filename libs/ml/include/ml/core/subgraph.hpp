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

#include "ml/core/graph.hpp"

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
class SubGraph : public Graph<T>, public ops::Ops<T>
{
public:
  using TensorType    = T;
  using VecTensorType = std::vector<std::shared_ptr<TensorType const>>;
  using SPType        = SubGraphSaveableParams<TensorType>;
  using OpPtrType     = std::shared_ptr<fetch::ml::ops::Ops<TensorType>>;

  void                    Forward(VecTensorType const &inputs, TensorType &output) override;
  std::vector<TensorType> Backward(VecTensorType const &inputs,
                                   TensorType const &   error_signal) override;

  std::shared_ptr<OpsSaveableParams> GetOpSaveableParams() override
  {
    auto gsp = this->GetGraphSaveableParams();

    auto sp    = std::make_shared<SPType>();
    auto g_ptr = std::dynamic_pointer_cast<GraphSaveableParams<TensorType>>(sp);
    *g_ptr     = gsp;

    sp->input_node_names = input_node_names_;
    sp->output_node_name = output_node_name_;

    return sp;
  }

protected:
  /**
   * Inserts a copy of the subgraph (with shared op ptrs where appropriate) into output_ptr
   * @param output_ptr shared_ptr to the new graph. Needs to not be the same as the old subgraph!
   */
  void InsertSharedCopy(std::shared_ptr<fetch::ml::ops::Ops<TensorType>> output_ptr)
  {
    if (output_ptr.get() == this)
    {
      throw ml::exceptions::InvalidMode("This needs to be called with a separate ptr.");
    }
    auto copyshare = std::dynamic_pointer_cast<SubGraph<TensorType>>(output_ptr);
    assert(copyshare);

    copyshare->input_node_names_ = input_node_names_;
    copyshare->output_node_name_ = output_node_name_;

    Graph<TensorType>::InsertSharedCopy(copyshare);
  }

public:
  /**
   * This function has to exist because it is declared in Op, but Subgraph cannnot create a
   * copy of itself so calling this will throw an error.
   * @param me Used for compatability
   * @return
   */
  std::shared_ptr<fetch::ml::ops::Ops<TensorType>> MakeSharedCopy(
      std::shared_ptr<fetch::ml::ops::Ops<TensorType>> me) override
  {
    FETCH_UNUSED(me);

    throw ml::exceptions::InvalidMode(
        "SubGraph cannot make a shared copy of itself because it is pure virtual.");
  }

  static constexpr char const *DESCRIPTOR = "SubGraph";

  void AddInputNode(std::string const &node_name);
  void SetOutputNode(std::string const &node_name);

protected:
  SubGraph() = default;

private:
  std::vector<std::string> input_node_names_;
  std::string              output_node_name_;
};

/**
 *
 * @tparam T
 * @param inputs
 * @param output
 * @return
 */
template <typename T>
void SubGraph<T>::Forward(VecTensorType const &inputs, TensorType &output)
{
  assert(inputs.size() == this->input_node_names_.size());
  for (uint64_t i(0); i < inputs.size(); ++i)
  {
    this->SetInput(input_node_names_[i], *(inputs.at(i)));
  }
  output = *(this->nodes_[output_node_name_]->Evaluate(this->is_training_));
}

template <typename T>
std::vector<T> SubGraph<T>::Backward(VecTensorType const &inputs, TensorType const &error_signal)
{
  assert(inputs.size() == this->input_node_names_.size());
  FETCH_UNUSED(inputs);

  std::vector<std::pair<Node<T> *, TensorType>> non_back_prop_err_signal =
      this->nodes_[output_node_name_]->BackPropagate(error_signal);
  std::vector<TensorType> back_prop_err_signal;

  // aggregate the error to each input, if there are more than one error signal for one input
  for (std::size_t i = 0; i < input_node_names_.size(); i++)
  {
    std::shared_ptr<Node<T>> node     = this->nodes_[input_node_names_[i]];
    bool                     is_first = true;
    for (auto const &grad : non_back_prop_err_signal)
    {
      if (grad.first == node.get())
      {
        if (is_first)
        {
          back_prop_err_signal.emplace_back(grad.second);
          is_first = false;
        }
        else
        {
          fetch::math::Add(back_prop_err_signal.at(i), grad.second, back_prop_err_signal.at(i));
        }
      }
    }
    // make sure every input node has at least one error signal
    assert(!is_first);
  }

  return back_prop_err_signal;
}

template <typename T>
void SubGraph<T>::AddInputNode(std::string const &node_name)
{
  input_node_names_.push_back(node_name);
}

template <typename T>
void SubGraph<T>::SetOutputNode(std::string const &node_name)
{
  output_node_name_ = node_name;
}

}  // namespace ml
}  // namespace fetch
