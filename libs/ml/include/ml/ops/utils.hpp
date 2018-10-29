#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018 Fetch.AI Limited
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

#include "math/free_functions/free_functions.hpp"
#include "ml/ops/derivatives/derivatives.hpp"

namespace fetch {
namespace ml {
namespace ops {

/**
 * The Dot method for ML variables. Applies a standard library Dot but also tracks parents and
 * updates gradients
 * @param a
 * @param b
 * @return
 */
template <typename VariablePtrType>
void DotImplementation(VariablePtrType cur_node)
{
  cur_node->data() =
      fetch::math::Dot(cur_node->prev[0]->data(), cur_node->prev[1]->data(), cur_node->threaded());
}

template <typename VariablePtrType, typename SessionType>
VariablePtrType Dot(VariablePtrType left, VariablePtrType right, SessionType &sess)
{
  // define the back_function (derivative)
  std::function<void(VariablePtrType)> b_fn = [](VariablePtrType cur_node) {
    fetch::ml::ops::derivatives::Dot(cur_node);
  };

  // define the forward function (i.e. the dot)
  std::function<void(VariablePtrType)> f_fn = [](VariablePtrType cur_node) {
    DotImplementation(cur_node);
  };

  // define the return variable with the Dot computation
  std::vector<std::size_t> out_shape     = {left->shape()[0], right->shape()[1]};
  bool                     is_leaf       = false;
  bool                     requires_grad = false;
  VariablePtrType ret = sess.Variable(out_shape, "Dot", f_fn, b_fn, is_leaf, requires_grad);

  ret->prev.push_back(left);
  ret->prev.push_back(right);

  return ret;
}

/**
 * The Dot method for ML variables. Applies a standard library Dot but also tracks parents and
 * updates gradients
 * @param a
 * @param b
 * @return
 */
template <typename VariablePtrType>
void AddBroadcastImplementation(VariablePtrType cur_node)
{
  assert(cur_node->prev.size() == 2);
  for (std::size_t i = 0; i < cur_node->prev[0]->data().shape()[0]; ++i)
  {
    for (std::size_t j = 0; j < cur_node->prev[0]->data().shape()[1]; ++j)
    {
      cur_node->data().Set(i, j,
                           cur_node->prev[0]->data().At(i, j) + cur_node->prev[1]->data().At(j));
    }
  }
  //    cur_node->data() = fetch::math::Add(cur_node->prev[0]->data(), cur_node->prev[1]->data());
}

template <typename VariablePtrType, typename SessionType>
VariablePtrType AddBroadcast(VariablePtrType left, VariablePtrType right, SessionType &sess)
{
  // define the back_function (derivative)
  std::function<void(VariablePtrType)> b_fn = [](VariablePtrType cur_node) {
    fetch::ml::ops::derivatives::AddBroadcast(cur_node);
  };

  // define the forward function (i.e. the dot)
  std::function<void(VariablePtrType)> f_fn = [](VariablePtrType cur_node) {
    AddBroadcastImplementation(cur_node);
  };

  // define the return variable with the Dot computation
  std::vector<std::size_t> out_shape     = left->shape();  // we always assume biases are on the RHS
  bool                     is_leaf       = false;
  bool                     requires_grad = false;
  VariablePtrType ret = sess.Variable(out_shape, "Add", f_fn, b_fn, is_leaf, requires_grad);

  ret->prev.push_back(left);
  ret->prev.push_back(right);

  return ret;
}

/**
 * op for Summing across an axis
 * @return
 */
template <typename VariablePtrType>
void ReduceSumImplementation(VariablePtrType cur_node)
{
  cur_node->data() = fetch::math::ReduceSum(cur_node->prev[0]->data(), cur_node->prev[1]->data());
}
template <typename VariableType, typename SessionType,
          typename VariablePtrType = std::shared_ptr<VariableType>>
VariablePtrType ReduceSum(std::shared_ptr<VariableType> left, std::size_t const &axis,
                          SessionType &sess)
{
  // define the derivative
  std::function<void(VariablePtrType)> b_fn = [](VariablePtrType cur_node) {
    fetch::ml::ops::derivatives::ReduceSum(cur_node);
  };

  // define the forward function (i.e. the dot)
  std::function<void(VariablePtrType)> f_fn = [](VariablePtrType cur_node) {
    ReduceSumImplementation(cur_node);
  };

  // define Variable of zeros to compare against
  VariablePtrType node_axis = SessionType::Zeroes({1, 1}, sess);
  node_axis->data()[0]      = static_cast<typename VariableType::ArrayType::Type>(axis);

  bool            is_leaf       = false;
  bool            requires_grad = false;
  VariablePtrType ret = sess.Variable(left->shape(), "Sum", f_fn, b_fn, is_leaf, requires_grad);

  ret->prev.push_back(left);
  ret->prev.push_back(node_axis);

  return ret;
}

/**
 *
 */

}  // namespace ops
}  // namespace ml
}  // namespace fetch
