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

#include "ml/core/node.hpp"
#include "ml/core/subgraph.hpp"
#include "ml/ops/ops.hpp"
#include "ml/ops/weights.hpp"
#include "ml/saveparams/saveable_params.hpp"

namespace fetch {
namespace ml {

using Shape       = fetch::math::SizeVector;
using ShapeVector = std::vector<fetch::math::SizeVector>;

/**
 * @brief A helper function for printing node's output shape
 */
static std::string OutputShapeAsString(const math::SizeVector &out_shape)
{
  std::stringstream ss;
  ss << " (out ";
  if (out_shape.empty())
  {
    ss << "[??] )";
    return ss.str();
  }
  ss << "[";
  for (auto const &dim : out_shape)
  {
    ss << " " << dim;
  }
  ss << " ])";
  return ss.str();
}

/**
 * @brief A helper function for printing node's input shape(s)
 */
static std::string InputShapesAsString(const std::vector<math::SizeVector> &in_shapes)
{
  std::stringstream ss;
  ss << " (in ";
  if (in_shapes.empty())
  {
    ss << "[[??]] )";
    return ss.str();
  }
  ss << "[";
  for (auto const &shape : in_shapes)
  {
    ss << "[";
    for (auto const &dim : shape)
    {
      ss << " " << dim;
    }
    ss << " ]";
  }
  ss << "])";
  return ss.str();
}

/**
 * returns the stored operation type and syncs it with operation type of
 * underlying Ops.
 * @return
 */
template <typename TensorType>
OpType Node<TensorType>::OperationType() const
{
  if (operation_type_ != op_ptr_->OperationType())
  {
    FETCH_LOG_ERROR(this->name_.c_str(), "Node operation type (" +
                                             std::to_string(static_cast<int>(operation_type_)) +
                                             ") and underlying Ops operation code (" +
                                             std::string(op_ptr_->Descriptor()) + ") mismatch!");
  }
  return operation_type_;
}

template <typename TensorType>
void Node<TensorType>::SetBatchOutputShape(const Shape &new_shape)
{
  op_ptr_->SetBatchOutputShape(new_shape);
}

template <typename TensorType>
void Node<TensorType>::SetBatchInputShapes(const ShapeVector &new_shapes)
{
  op_ptr_->SetBatchInputShapes(new_shapes);
}

template <typename TensorType>
const ShapeVector &Node<TensorType>::BatchInputShapes() const
{
  return op_ptr_->BatchInputShapes();
}

/**
 * @brief BatchOutputShape computes an output shape of the Node, if only 1 data slice (e.g.
 * with batch size == 1) is provided to the Graph input. If there is no cached output shape,
 * the method is recursively called until either a cached shape or input Node is encountered.
 * @return vector of SizeType.
 */
template <typename TensorType>
Shape Node<TensorType>::BatchOutputShape()
{
  Shape const &candidate = op_ptr_->BatchOutputShape();

  // If the underlying Op shape is already known (cached) this shape can be returned immediately.
  if (!candidate.empty())
  {
    if (OperationType() == OpType::OP_PLACEHOLDER)
    {
      FETCH_LOG_INFO(name_.c_str(), "Shape deduction reached a placeholder input node : " +
                                        this->name_ + " " + OutputShapeAsString(candidate));
    }
    if (OperationType() == OpType::OP_WEIGHTS)
    {
      FETCH_LOG_INFO(name_.c_str(), "Shape deduction reached weights node : " + this->name_ + " " +
                                        OutputShapeAsString(candidate));
    }
    return candidate;
  }

  if (input_nodes_.empty())
  {
    // If there is no input nodes, and underlying Op's shape is not known - the Graph,
    // probably, is incorrect; however, some nodes (like Label placeholder) could have
    // empty shape without causing Graph malfunction or shape deduction failure.
    FETCH_LOG_ERROR(
        name_.c_str(),
        " Shape deduction reached a leaf Node with empty/unknown shape : " + this->name_);

    return candidate;
  }

  // If Ops shape is unknown, but there are input nodes - they could be asked for their shapes.
  ShapeVector input_shapes;
  for (auto const &i : input_nodes_)
  {
    auto input_node_ptr = i.lock();
    if (!input_node_ptr)
    {
      throw std::runtime_error("Unable to lock weak pointer.");
    }

    // If there are valid input nodes, make a deeper recursive call to each of the previous nodes.
    Shape const in_shape = input_node_ptr->BatchOutputShape();

    if (in_shape.empty())
    {
      if (input_node_ptr->OperationType() != OpType::OP_PLACEHOLDER)
      {
        FETCH_LOG_INFO(name_.c_str(),
                       "Got an empty shape as return from non-placeholder layer! : " +
                           input_node_ptr->GetNodeName());
      }
      continue;
    }

    input_shapes.emplace_back(in_shape);
  }

  // If there is no one valid (non-empty) input shape, shape deduction can not go further.
  if (input_shapes.empty())
  {
    FETCH_LOG_ERROR(name_.c_str(), "Shape deduction failed on " + this->name_ +
                                       " : only empty shapes were received as Input ones.");
    return candidate;
  }

  // When all valid (non-empty) input shapes are collected, it is possbile to compute
  // an output shape of this node's Ops.
  Shape const my_out_shape = op_ptr_->ComputeBatchOutputShape(input_shapes);

  if (my_out_shape.empty())
  {
    FETCH_LOG_ERROR(name_.c_str(), "Shape deduction failed on " + this->name_ +
                                       " : unable to compute underlying Ops output shape.");
  }
  else
  {
    FETCH_LOG_INFO(name_.c_str(), InputShapesAsString(op_ptr_->BatchInputShapes()) + "->" +
                                      OutputShapeAsString(op_ptr_->BatchOutputShape()));
  }

  // After all shapes for current Node are deduced, shape-dependent Ops could be
  // updated and their initialisation completed.
  op_ptr_->CompleteConstruction();

  return my_out_shape;
}

/**
 * Constructs and returns a NodeSaveableParams object allowing serialisation
 * @tparam T
 * @return
 */
template <typename TensorType>
std::shared_ptr<typename Node<TensorType>::SPType> Node<TensorType>::GetNodeSaveableParams() const
{
  auto sp_ptr = std::make_shared<SPType>();

  sp_ptr->name           = name_;
  sp_ptr->operation_type = operation_type_;
  sp_ptr->op_save_params = op_ptr_->GetOpSaveableParams();

  return sp_ptr;
}

/**
 * returns a vector of all nodes which provide input to this node
 * @tparam TensorType tensor
 * @return vector of reference_wrapped tensors
 */
template <class TensorType>
typename Node<TensorType>::VecTensorType Node<TensorType>::GatherInputs() const
{
  VecTensorType inputs;
  for (auto const &i : input_nodes_)
  {
    if (auto ptr = i.lock())
    {
      inputs.push_back(ptr->Evaluate(op_ptr_->IsTraining()));
    }
    else
    {
      throw std::runtime_error("Unable to lock weak pointer.");
    }
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
template <typename TensorType>
std::shared_ptr<TensorType> Node<TensorType>::Evaluate(bool is_training)
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

    if (math::state_division_by_zero<DataType>())
    {
      throw std::runtime_error("Division by zero encountered in Node::Evaluate");
    }
    if (math::state_infinity<DataType>())
    {
      throw std::runtime_error("Infinity encountered in Node::Evaluate");
    }
    if (math::state_nan<DataType>())
    {
      throw std::runtime_error("NaN encountered in Node::Evaluate");
    }

    assert(!math::state_overflow<DataType>());
  }

  return std::make_shared<TensorType>(cached_output_);
}

/**
 * Recursively backpropagates error_signal through this node to all input nodes
 * @tparam T the tensor type
 * @tparam O the operation class
 * @param error_signal the error signal to backpropagate
 * @return
 */
template <typename TensorType>
typename Node<TensorType>::NodeErrorMapType Node<TensorType>::BackPropagate(
    TensorType const &error_signal)
{
  NodeErrorMapType ret;

  // gather inputs and backprop for this node
  std::vector<TensorType> error_signals = op_ptr_->Backward(GatherInputs(), error_signal);
  assert(error_signals.size() == GatherInputs().size() || GatherInputs().empty());

  if (input_nodes_.empty())
  {
    // if this node has no inputs assign error signal to this node
    ret[this] = error_signals;
  }
  else
  {
    // otherwise backpropagate on the input nodes
    auto bp_it = error_signals.begin();
    for (auto &i : input_nodes_)
    {
      if (auto ptr = i.lock())
      {
        auto ret_err_sig = ptr->BackPropagate(*bp_it);
        ret.insert(ret_err_sig.begin(), ret_err_sig.end());
      }
      else
      {
        throw std::runtime_error("Unable to lock weak pointer.");
      }

      ++bp_it;
    }
  }

  if (math::state_division_by_zero<DataType>())
  {
    throw std::runtime_error("Division by zero encountered in Node::BackPropagate");
  }
  if (math::state_infinity<DataType>())
  {
    throw std::runtime_error("Infinity encountered in Node::BackPropagate");
  }
  if (math::state_nan<DataType>())
  {
    throw std::runtime_error("NaN encountered in Node::BackPropagate");
  }

  assert(!math::state_overflow<DataType>());
  return ret;
}
/**
 * Resets input and output node ptr containers. Useful for graph decompiling.
 * @tparam T
 */
template <typename TensorType>
void Node<TensorType>::ResetInputsAndOutputs()
{
  input_nodes_.clear();
  outputs_.clear();
}

/**
 * registers a node as an input to this node
 * @tparam T tensor type
 * @tparam O operation class
 * @param i pointer to the input node
 */
template <typename TensorType>
void Node<TensorType>::AddInput(NodeWeakPtrType const &i)
{
  input_nodes_.push_back(i);
}

/**
 * registers a node as an input to this node
 * @tparam T tensor type
 * @tparam O operation class
 * @param i pointer to the input node
 */
template <typename TensorType>
std::vector<std::string> Node<TensorType>::GetInputNames()
{
  std::vector<std::string> ret{};
  for (auto const &input_node : input_nodes_)
  {
    if (auto ptr = input_node.lock())
    {
      ret.emplace_back(ptr->name_);
    }
    else
    {
      throw std::runtime_error("Unable to lock weak pointer.");
    }
  }
  return ret;
}

/**
 * registers a node as an output to this node
 * @tparam T tensor type
 * @tparam O operation class
 * @param o pointer to the output node
 */
template <typename TensorType>
void Node<TensorType>::AddOutput(NodeWeakPtrType const &o)
{
  outputs_.push_back(o);
}

/**
 * gets all registered outputs of this node
 * @tparam T tensor type
 * @tparam O operation class
 * @return vector of pointers to output nodes
 */
template <typename TensorType>
std::vector<typename Node<TensorType>::NodeWeakPtrType> const &Node<TensorType>::GetOutputs() const
{
  return outputs_;
}

/**
 * Resets the cache status of this node depending on whether the input size has changed
 * @tparam T tensor type
 * @tparam O operation class
 * @param input_size_changed boolean indicating whether the input size changed
 */
template <typename TensorType>
void Node<TensorType>::ResetCache(bool input_size_changed)
{
  if (cached_output_status_ != CachedOutputState::CHANGED_SIZE)
  {
    cached_output_status_ =
        input_size_changed ? CachedOutputState::CHANGED_SIZE : CachedOutputState::CHANGED_CONTENT;
  }
}

/**
 * Sets the saveable params back to the node
 * @tparam TensorType
 * @tparam OperationType
 * @param nsp
 * @param op_ptr
 */
template <typename TensorType>
void Node<TensorType>::SetNodeSaveableParams(NodeSaveableParams<TensorType> const &       nsp,
                                             std::shared_ptr<ops::Ops<TensorType>> const &op_ptr)
{
  name_                 = nsp.name;
  cached_output_status_ = CachedOutputState::CHANGED_SIZE;
  operation_type_       = nsp.operation_type;
  op_ptr_               = op_ptr;
}

///////////////////////////////
/// EXPLICIT INSTANTIATIONS ///
///////////////////////////////

template class Node<math::Tensor<int8_t>>;
template class Node<math::Tensor<int16_t>>;
template class Node<math::Tensor<int32_t>>;
template class Node<math::Tensor<int64_t>>;
template class Node<math::Tensor<float>>;
template class Node<math::Tensor<double>>;
template class Node<math::Tensor<fixed_point::fp32_t>>;
template class Node<math::Tensor<fixed_point::fp64_t>>;
template class Node<math::Tensor<fixed_point::fp128_t>>;

}  // namespace ml
}  // namespace fetch
