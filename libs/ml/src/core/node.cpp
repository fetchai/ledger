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

namespace fetch {
namespace ml {

using Shape       = fetch::math::SizeVector;
using ShapeVector = std::vector<fetch::math::SizeVector>;

/**
 * returns the stored operation type and syncs it with operation type of
 * underlying Ops.
 * @return
 */
template <typename TensorType>
OpType Node<TensorType>::OperationType()
{
  if (operation_type_ != op_ptr_->OperationType())
  {
    FETCH_LOG_ERROR(this->name_.c_str(), "Node operation type (" +
                                             std::to_string(static_cast<int>(operation_type_)) +
                                             ") and underlying Ops operation code (" +
                                             std::string(op_ptr_->Descriptor()) + ") mismatch!");
    operation_type_ = op_ptr_->OperationType();
  }
  return operation_type_;
}

template <typename TensorType>
bool Node<TensorType>::HasValidCache()
{
  return static_cast<bool>(cached_output_status_ == CachedOutputState::VALID_CACHE);
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
const ShapeVector &Node<TensorType>::BatchInputShapes()
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
  bool const   op_is_subgraph =
      (std::dynamic_pointer_cast<std::shared_ptr<SubGraph<TensorType>>>(op_ptr_) != nullptr);

  // If the underlying Op is simple (e.g. not a SubGraph) and its shape is already known (cached)
  // this shape can be returned immediately.
  if (!candidate.empty() && !op_is_subgraph)
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

  // If there is no input nodes, shape deduction can not go further.
  if (input_nodes_.empty())
  {
    if (candidate.empty())
    {
      // If there is no input nodes, but underlying Op's shape is not known - the Graph
      // probably is incorrect.
      FETCH_LOG_ERROR(name_.c_str(),
                      "Shape deduction reached a Graph leaf with empty shape " + this->name_);
    }
    return candidate;
  }

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
  // an output shape of this node.
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
  op_ptr_->CompleteInitialisation();

  return my_out_shape;
}

///////////////////////////////
/// EXPLICIT INSTANTIATIONS ///
///////////////////////////////

template class Node<math::Tensor<int8_t>>;
template class Node<math::Tensor<int16_t>>;
template class Node<math::Tensor<int32_t>>;
template class Node<math::Tensor<int64_t>>;
template class Node<math::Tensor<uint8_t>>;
template class Node<math::Tensor<uint16_t>>;
template class Node<math::Tensor<uint32_t>>;
template class Node<math::Tensor<uint64_t>>;
template class Node<math::Tensor<float>>;
template class Node<math::Tensor<double>>;
template class Node<math::Tensor<fixed_point::fp32_t>>;
template class Node<math::Tensor<fixed_point::fp64_t>>;
template class Node<math::Tensor<fixed_point::fp128_t>>;

/**
 * @brief A helper function for printing layer's output shape
 */
std::string OutputShapeAsString(const math::SizeVector &out_shape)
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
 * @brief A helper function for printing layer's input shape(s)
 */
std::string InputShapesAsString(const std::vector<math::SizeVector> &in_shapes)
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

}  // namespace ml
}  // namespace fetch
