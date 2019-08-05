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
#include "ml/core/node.hpp"

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
  using ArrayType     = T;
  using VecTensorType = std::vector<std::shared_ptr<ArrayType const>>;
  using SPType        = SubGraphSaveableParams<ArrayType>;

  void                   Forward(VecTensorType const &inputs, ArrayType &output) override;
  std::vector<ArrayType> Backward(VecTensorType const &inputs,
                                  ArrayType const &    error_signal) override;

  template <typename X>
  static std::shared_ptr<SubGraph<X>> BuildSubGraph(SubGraphSaveableParams<X> const &sgsp)
  {
    auto ret = std::make_shared<SubGraph<X>>();

    ret->input_node_names_ = sgsp.input_node_names;
    ret->output_node_name_ = sgsp.output_node_name;

    return ret;

    // TODO - this must have already been done by the utilities
    //    // first load graph based on upcasting subpgrah saveparams to graph saveparams
    //    auto ptr_graph_saveable_params = std::make_shared<typename Graph<T>::SPType>(sgsp);
    //    auto graph_ptr = fetch::ml::utilities::LoadGraph<ArrayType,
    //    Graph<T>>(ptr_graph_saveable_params);
    //
    //    // overwrite base class with graph loaded from save params
    //    static_cast<Graph<T>>(*this) = *graph_ptr;
    //
    //    // assign
    //    input_node_names_ = sgsp.input_node_names;
    //    output_node_name_ = sgsp.output_node_name;
  }

  std::shared_ptr<SaveableParamsInterface> GetOpSaveableParams() override
  {
    auto gsp = this->GetGraphSaveableParams();

    auto sp    = std::make_shared<SPType>();
    auto g_ptr = std::dynamic_pointer_cast<GraphSaveableParams<ArrayType>>(sp);
    *g_ptr     = gsp;

    sp->input_node_names = input_node_names_;
    sp->output_node_name = output_node_name_;

    return sp;
  }

  static constexpr char const *DESCRIPTOR = "SubGraph";

protected:
  void AddInputNode(std::string const &node_name);
  void SetOutputNode(std::string const &node_name);

protected:
  SubGraph() = default;

public:
  std::vector<std::string> input_node_names_;

private:
  std::string output_node_name_;
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
  assert(inputs.size() == this->input_node_names_.size());
  for (uint64_t i(0); i < inputs.size(); ++i)
  {
    this->SetInput(input_node_names_[i], *(inputs.at(i)));
  }
  output = *(this->nodes_[output_node_name_]->Evaluate(this->is_training_));
}

template <typename T>
std::vector<T> SubGraph<T>::Backward(VecTensorType const &inputs, ArrayType const &error_signal)
{
  assert(inputs.size() == this->input_node_names_.size());
  FETCH_UNUSED(inputs);
  std::vector<std::pair<Node<T> *, ArrayType>> non_back_prop_err_signal =
      this->nodes_[output_node_name_]->BackPropagateSignal(error_signal);
  std::vector<ArrayType> back_prop_err_signal;

  for (std::string const &s : input_node_names_)
  {
    std::shared_ptr<Node<T>> node = this->nodes_[s];
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
  input_node_names_.push_back(node_name);
}

template <typename T>
void SubGraph<T>::SetOutputNode(std::string const &node_name)
{
  output_node_name_ = node_name;
}

}  // namespace ml
}  // namespace fetch
