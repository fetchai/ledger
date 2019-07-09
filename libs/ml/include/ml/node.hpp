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

#include "core/logger.hpp"
#include "ops/ops.hpp"

#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace fetch {
namespace ml {

template <class T>
class NodeInterface
{
public:
  using ArrayType   = T;
  using NodePtrType = std::shared_ptr<NodeInterface<T>>;

  virtual ArrayType &                                           Evaluate(bool is_training)      = 0;
  virtual void                                                  AddInput(NodePtrType const &i)  = 0;
  virtual void                                                  AddOutput(NodePtrType const &i) = 0;
  virtual std::vector<std::pair<NodeInterface<T> *, ArrayType>> BackPropagateSignal(
      ArrayType const &error_signal)                                          = 0;
  virtual void                            ResetCache(bool input_size_changed) = 0;
  virtual std::vector<NodePtrType> const &GetOutputs() const                  = 0;
};

template <class T, class O>
class Node : public NodeInterface<T>, public O
{
private:
  enum class CachedOutputState
  {
    VALID_CACHE,
    CHANGED_CONTENT,
    CHANGED_SIZE
  };

public:
  using ArrayType   = T;
  using NodePtrType = std::shared_ptr<NodeInterface<T>>;

  template <typename... Params>
  Node(std::string const name, Params... params)
    : O(params...)
    , name_(std::move(name))
    , cached_output_status_(CachedOutputState::CHANGED_SIZE)
  {}

  virtual ~Node() = default;

  std::vector<std::reference_wrapper<const ArrayType>>          GatherInputs() const;
  virtual ArrayType &                                           Evaluate(bool is_training);
  virtual std::vector<std::pair<NodeInterface<T> *, ArrayType>> BackPropagateSignal(
      ArrayType const &error_signal);

  void                                    AddInput(NodePtrType const &i);
  void                                    AddOutput(NodePtrType const &o);
  virtual std::vector<NodePtrType> const &GetOutputs() const;
  virtual void                            ResetCache(bool input_size_changed);

private:
  std::vector<NodePtrType> input_nodes_;
  std::vector<NodePtrType> outputs_;
  std::string              name_;
  ArrayType                cached_output_;
  CachedOutputState        cached_output_status_;
};

/**
 * returns a vector of all nodes which provide input to this node
 * @tparam ArrayType tensor
 * @return vector of reference_wrapped tensors
 */
template <class T, class O>
std::vector<std::reference_wrapper<const T>> Node<T, O>::GatherInputs() const
{
  std::vector<std::reference_wrapper<const ArrayType>> inputs;
  for (auto const &i : input_nodes_)
  {
    inputs.push_back(i->Evaluate(this->is_training_));
  }
  return inputs;
}

/**
 * Returns the result of a forward evaluation of this node. If that's already been
 * computed this is cheap; if not then Forward is called as necessary if the output
 * size has not been updated since last used. This also must be changed and
 * recalculated as necessary
 * @tparam T tensor type
 * @tparam O operation class
 * @return the tensor with the forward result
 */
template <typename T, class O>
T &Node<T, O>::Evaluate(bool is_training)
{

  this->SetTraining(is_training);

  if (cached_output_status_ != CachedOutputState::VALID_CACHE)
  {
    std::vector<std::reference_wrapper<const ArrayType>> inputs = GatherInputs();

    if (cached_output_status_ == CachedOutputState::CHANGED_SIZE)
    {
      auto output_shape = this->ComputeOutputShape(inputs);

      if (cached_output_.shape() !=
          output_shape)  // make shape compatible right before we do the forwarding
      {
        cached_output_.ResizeFromShape(output_shape);
      }
    }
    this->Forward(inputs, cached_output_);
    cached_output_status_ = CachedOutputState::VALID_CACHE;
  }

  return cached_output_;
}

/**
 * Recursively backpropagates errorsignal through this node to all input nodes
 * @tparam T the tensor type
 * @tparam O the operation class
 * @param error_signal the error signal to backpropagate
 * @return
 */
template <typename T, class O>
std::vector<std::pair<NodeInterface<T> *, T>> Node<T, O>::BackPropagateSignal(
    ArrayType const &error_signal)
{
  std::vector<std::reference_wrapper<const ArrayType>> inputs = GatherInputs();
  std::vector<ArrayType> back_propagated_error_signals = this->Backward(inputs, error_signal);
  std::vector<std::pair<NodeInterface<T> *, ArrayType>> non_back_propagated_error_signals;
  assert(back_propagated_error_signals.size() == inputs.size() || inputs.empty());

  auto bp_it = back_propagated_error_signals.begin();
  for (auto &i : input_nodes_)
  {
    auto ret = i->BackPropagateSignal(*bp_it);
    non_back_propagated_error_signals.insert(non_back_propagated_error_signals.end(), ret.begin(),
                                             ret.end());
    ++bp_it;
  }

  // If no input to backprop to, return gradient to caller
  // This is used to propagate outside of a SubGraph
  // The SubGraph has no knowledge of the rest of the network,
  // so it sends its unpropagated gradient to its wrapper node that will forward them out
  if (input_nodes_.empty())
  {
    for (auto g : back_propagated_error_signals)
    {
      non_back_propagated_error_signals.push_back(std::make_pair(this, g));
    }
  }
  return non_back_propagated_error_signals;
}

/**
 * registers a node as an input to this node
 * @tparam T tensor type
 * @tparam O operation class
 * @param i pointer to the input node
 */
template <typename T, class O>
void Node<T, O>::AddInput(NodePtrType const &i)
{
  input_nodes_.push_back(i);
}

/**
 * registers a node as an output to this node
 * @tparam T tensor type
 * @tparam O operation class
 * @param o pointer to the output node
 */
template <typename T, class O>
void Node<T, O>::AddOutput(NodePtrType const &o)
{
  outputs_.push_back(o);
}

/**
 * gets all registered outputs of this node
 * @tparam T tensor type
 * @tparam O operation class
 * @return vector of pointers to output nodes
 */
template <typename T, class O>
std::vector<typename Node<T, O>::NodePtrType> const &Node<T, O>::GetOutputs() const
{
  return outputs_;
}

/**
 * Resets the cache status of this node depending on whether the input size has changed
 * @tparam T tensor type
 * @tparam O operation class
 * @param input_size_changed boolean indicating whether the input size changed
 */
template <typename T, class O>
void Node<T, O>::ResetCache(bool input_size_changed)
{
  cached_output_status_ =
      input_size_changed ? CachedOutputState::CHANGED_SIZE : CachedOutputState::CHANGED_CONTENT;
}

}  // namespace ml
}  // namespace fetch
