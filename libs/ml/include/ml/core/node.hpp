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
#include "ml/ops/op_interface.hpp"
#include "ml/ops/ops.hpp"
#include "ml/saveparams/saveable_params.hpp"

#include "ml/ops/abs.hpp"

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace fetch {
namespace ml {

// template <class T>
// class NodeInterface
//{
// public:
//  using ArrayType   = T;
//  using NodePtrType = std::shared_ptr<NodeInterface<T>>;
//
//  virtual std::shared_ptr<T>                                    Evaluate(bool is_training)      =
//  0; virtual void                                                  AddInput(NodePtrType const &i)
//  = 0; virtual void                                                  AddOutput(NodePtrType const
//  &i) = 0; virtual std::vector<std::pair<NodeInterface<T> *, ArrayType>> BackPropagateSignal(
//      ArrayType const &error_signal)                                                   = 0;
//  virtual void                                     ResetCache(bool input_size_changed) = 0;
//  virtual std::vector<NodePtrType> const &         GetOutputs() const                  = 0;
//  virtual std::shared_ptr<SaveableParamsInterface> GetNodeSaveableParams()             = 0;
//};

// template <class T, class O>
// class Node : public NodeInterface<T>
template <typename T>
class Node
{
private:
  enum class CachedOutputState
  {
    VALID_CACHE,
    CHANGED_CONTENT,
    CHANGED_SIZE
  };

public:
  using ArrayType     = T;
  using NodePtrType   = std::shared_ptr<Node<T>>;
  using VecTensorType = typename fetch::ml::ops::Ops<T>::VecTensorType;

  //  template <typename... Params>
  Node(OpType const &operation_type, std::string name,
       std::function<std::shared_ptr<ops::Ops<ArrayType>>()> const &constructor)
    : name_(std::move(name))
    , cached_output_status_(CachedOutputState::CHANGED_SIZE)
    , operation_type_(operation_type)
  {
    op_ptr_ = constructor();
  }

  //  //  template <typename... Params>
  //  Node(OpType const &operation_type, std::string name, std::shared_ptr<ops::Ops<ArrayType>>
  //  op_ptr)
  //    : name_(std::move(name))
  //    , cached_output_status_(CachedOutputState::CHANGED_SIZE)
  //    , operation_type_(operation_type)
  //    , op_ptr_(op_ptr)
  //  {}

  //  template <typename... Params>

  /**
   * This method CANNOT be extended to include making any op that is a layer
   */
  Node(OpType const &operation_type, std::string name)
    : name_(std::move(name))
    , cached_output_status_(CachedOutputState::CHANGED_SIZE)
    , operation_type_(operation_type)
  {
    switch (operation_type)
    {
    case ops::Abs<ArrayType>::OpCode():
      op_ptr_ = std::make_shared<fetch::ml::ops::Abs<ArrayType>>();
      break;
    case ops::PlaceHolder<ArrayType>::OpCode():
      op_ptr_ = std::make_shared<fetch::ml::ops::PlaceHolder<ArrayType>>();
      break;
    case ops::Weights<ArrayType>::OpCode():
      op_ptr_ = std::make_shared<fetch::ml::ops::Weights<ArrayType>>();
      break;
    default:
      throw std::runtime_error("unknown node type");
    }
  }

  std::shared_ptr<SaveableParamsInterface> GetNodeSaveableParams()
  {
    return op_ptr_->GetOpSaveableParams();
  }

  ~Node() = default;

  VecTensorType                                GatherInputs() const;
  std::shared_ptr<T>                           Evaluate(bool is_training);
  std::vector<std::pair<Node<T> *, ArrayType>> BackPropagateSignal(ArrayType const &error_signal);

  void                            AddInput(NodePtrType const &i);
  void                            AddOutput(NodePtrType const &o);
  std::vector<NodePtrType> const &GetOutputs() const;
  void                            ResetCache(bool input_size_changed);

  std::string const &GetNodeName()
  {
    return name_;
  }

  std::shared_ptr<ops::Ops<T>> GetOp()
  {
    return op_ptr_;
  }

  OpType GetOpType()
  {
    return operation_type_;
  }

private:
  std::vector<NodePtrType> input_nodes_;
  std::vector<NodePtrType> outputs_;
  std::string              name_;
  ArrayType                cached_output_;
  CachedOutputState        cached_output_status_;

  OpType                       operation_type_;
  std::shared_ptr<ops::Ops<T>> op_ptr_;
};

/**
 * returns a vector of all nodes which provide input to this node
 * @tparam ArrayType tensor
 * @return vector of reference_wrapped tensors
 */
template <class T>
typename Node<T>::VecTensorType Node<T>::GatherInputs() const
{
  VecTensorType inputs;
  for (auto const &i : input_nodes_)
  {
    inputs.push_back(i->Evaluate(op_ptr_->IsTraining()));
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
template <typename T>
std::shared_ptr<T> Node<T>::Evaluate(bool is_training)
{

  op_ptr_->SetTraining(is_training);

  if (cached_output_status_ != CachedOutputState::VALID_CACHE)
  {
    VecTensorType inputs = GatherInputs();

    if (cached_output_status_ == CachedOutputState::CHANGED_SIZE)
    {
      auto output_shape = op_ptr_->ComputeOutputShape(inputs);

      if (cached_output_.shape() !=
          output_shape)  // make shape compatible right before we do the forwarding
      {
        cached_output_.Reshape(output_shape);
      }
    }
    op_ptr_->Forward(inputs, cached_output_);
    cached_output_status_ = CachedOutputState::VALID_CACHE;
  }

  return std::make_shared<T>(cached_output_);
}

/**
 * Recursively backpropagates errorsignal through this node to all input nodes
 * @tparam T the tensor type
 * @tparam O the operation class
 * @param error_signal the error signal to backpropagate
 * @return
 */
template <typename T>
std::vector<std::pair<Node<T> *, T>> Node<T>::BackPropagateSignal(ArrayType const &error_signal)
{
  VecTensorType          inputs                        = GatherInputs();
  std::vector<ArrayType> back_propagated_error_signals = op_ptr_->Backward(inputs, error_signal);
  std::vector<std::pair<Node<T> *, ArrayType>> non_back_propagated_error_signals;
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
template <typename T>
void Node<T>::AddInput(NodePtrType const &i)
{
  input_nodes_.push_back(i);
}

/**
 * registers a node as an output to this node
 * @tparam T tensor type
 * @tparam O operation class
 * @param o pointer to the output node
 */
template <typename T>
void Node<T>::AddOutput(NodePtrType const &o)
{
  outputs_.push_back(o);
}

/**
 * gets all registered outputs of this node
 * @tparam T tensor type
 * @tparam O operation class
 * @return vector of pointers to output nodes
 */
template <typename T>
std::vector<typename Node<T>::NodePtrType> const &Node<T>::GetOutputs() const
{
  return outputs_;
}

/**
 * Resets the cache status of this node depending on whether the input size has changed
 * @tparam T tensor type
 * @tparam O operation class
 * @param input_size_changed boolean indicating whether the input size changed
 */
template <typename T>
void Node<T>::ResetCache(bool input_size_changed)
{
  cached_output_status_ =
      input_size_changed ? CachedOutputState::CHANGED_SIZE : CachedOutputState::CHANGED_CONTENT;
}

}  // namespace ml
}  // namespace fetch
