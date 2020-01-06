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

namespace fetch {
namespace ml {

using Shape = fetch::math::SizeVector;

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
Shape Node<TensorType>::SliceOutputShape()
{
  // Returned cached shape result if available;
  Shape const candidate     = op_ptr_->SliceOutputShape();
  bool const  i_am_subgraph = OperationType() == OpType::LAYER_FULLY_CONNECTED;  // DEBUG! REMOVEME
  if (!candidate.empty() && !i_am_subgraph)
  {
    if (input_nodes_.empty() &&
        (OperationType() == OpType::OP_PLACEHOLDER || OperationType() == OpType::OP_WEIGHTS))
    {
      FETCH_LOG_INFO(name_.c_str(), "Shape deduction reached an leaf node : " + this->name_ + " " +
                                        op_ptr_->OutputShapeAsString());
    }

    return candidate;
  }

  if (input_nodes_.empty())
  {
    FETCH_LOG_INFO(name_.c_str(), "Shape deduction reached a Graph leaf : " + this->name_);
    return candidate;
  }

  ShapeVector input_shapes;
  for (auto const &i : input_nodes_)
  {
    auto node_ptr = i.lock();
    if (!node_ptr)
    {
      throw std::runtime_error("Unable to lock weak pointer.");
    }
    // Deeper recursive call.
    auto const in_shape = node_ptr->SliceOutputShape();
    if (!in_shape.empty())
    {
      input_shapes.emplace_back(in_shape);
    }
    else
    {
      // TODO(VH): else (if there _is_ an empty shape among inputs) what?
      if (node_ptr->OperationType() != OpType::OP_PLACEHOLDER)
      {
        FETCH_LOG_INFO(name_.c_str(),
                       "Got an empty shape as return from non-placeholder layer! : " +
                           node_ptr->GetNodeName());
      }
    }
  }

  if (!input_shapes.empty())
  {
    op_ptr_->ComputeSliceOutputShape(input_shapes);
    FETCH_LOG_INFO(name_.c_str(),
                   op_ptr_->InputShapesAsString() + "->" + op_ptr_->OutputShapeAsString());
  }
  else
  {
    FETCH_LOG_ERROR(name_.c_str(), "Shape deduction failed on " + this->name_ +
                                       " : only empty shapes were received as Input ones.");
    return candidate;
  }

  Shape const ops_out_shape = op_ptr_->SliceOutputShape();
  if (ops_out_shape.empty())
  {
    // throw an error: invalid calcs from a previous layer.
    FETCH_LOG_ERROR(name_.c_str(), "Shape deduction failed on " + this->name_ +
                                       " : unable to compute underlying Ops output shape.");
  }

  // After all shapes for current Node are deduced, shape-dependent Ops could be
  // updated and their initialisation completed.
  op_ptr_->CompleteInitialisation();
  return ops_out_shape;
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

}  // namespace ml
}  // namespace fetch
