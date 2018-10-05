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

template <typename T, typename C>
class ShapeLessArray;
template <typename T, typename C>
class NDArray;
template <typename T, typename C>
class NDArrayIterator;

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
  cur_node->data() = fetch::math::Dot(cur_node->prev[0]->data(), cur_node->prev[1]->data());
};

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
  std::vector<std::size_t> out_shape = {left->shape()[0], right->shape()[1]};
  VariablePtrType          ret       = sess.Variable(out_shape, "Dot", f_fn, b_fn, false);

  ret->prev.push_back(left);
  ret->prev.push_back(right);

  return ret;
}

/**
 * The rectified linear unit returns the elementwise maximum of 0 and y
 * @tparam ARRAY_TYPE
 * @tparam T
 * @param y
 * @param ret
 */
template <typename VariablePtrType>
void ReluImplementation(VariablePtrType cur_node)
{
  // we assume that prev[1] hold the Variable full of zeros that was previously defined
  cur_node->data() = fetch::math::Maximum(cur_node->prev[0]->data(), cur_node->prev[1]->data());
}
template <typename VariablePtrType, typename SessionType>
VariablePtrType Relu(VariablePtrType left, SessionType &sess)
{
  // define the derivative
  std::function<void(VariablePtrType)> b_fn = [](VariablePtrType cur_node) {
    fetch::ml::ops::derivatives::Relu(cur_node);
  };

  // define the forward function (i.e. the dot)
  std::function<void(VariablePtrType)> f_fn = [](VariablePtrType cur_node) {
    ReluImplementation(cur_node);
  };

  // define Variable of zeros to compare against
  VariablePtrType zeros = SessionType::Zeroes(left->shape(), sess);

  // define the return variable with the Relu computation
  VariablePtrType ret = sess.Variable(left->shape(), "Relu", f_fn, b_fn, false);

  ret->prev.push_back(left);
  ret->prev.push_back(zeros);

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
template <typename VariablePtrType, typename SessionType>
VariablePtrType ReduceSum(VariablePtrType left, std::size_t const &axis, SessionType &sess)
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
  node_axis->data()[0]      = axis;

  // define the return variable with the Relu computation
  std::vector<std::size_t> grad_shape = {left->shape()[0], left->shape()[1]};
  std::vector<std::size_t> new_shape{grad_shape};
  if (axis == 0)
  {
    new_shape[0] = 1;
  }
  else
  {
    new_shape[1] = 1;
  }

  VariablePtrType ret = sess.Variable(new_shape, grad_shape, "Sum", f_fn, b_fn, false);

  ret->prev.push_back(left);
  ret->prev.push_back(node_axis);

  return ret;
}

/*
 * Op for mean squared error
 */
template <typename VariablePtrType>
void MSEImplementation(VariablePtrType cur_node)
{
  cur_node->data() =
      fetch::math::MeanSquareError(cur_node->prev[0]->data(), cur_node->prev[1]->data());
}
template <typename VariablePtrType, typename SessionType>
VariablePtrType MeanSquareError(VariablePtrType left, VariablePtrType right, SessionType &sess)
{
  // define the derivative
  std::function<void(VariablePtrType)> b_fn = [](VariablePtrType cur_node) {
    fetch::ml::ops::derivatives::MeanSquareError(cur_node);
  };

  // define the forward function (i.e. the dot)
  std::function<void(VariablePtrType)> f_fn = [](VariablePtrType cur_node) {
    MSEImplementation(cur_node);
  };

  // define the return variable with the Dot computation
  std::vector<std::size_t> new_shape{left->shape()[0], 1};
  VariablePtrType          ret = sess.Variable(new_shape, "MSE", f_fn, b_fn, false);

  ret->prev.push_back(left);
  ret->prev.push_back(right);

  return ret;
}

/*
 * Cross entropy loss Op
 */
template <typename VariablePtrType>
void CELImplementation(VariablePtrType cur_node)
{
  cur_node->data() =
      fetch::math::CrossEntropyLoss(cur_node->prev[0]->data(), cur_node->prev[1]->data());
}
template <typename VariablePtrType, typename SessionType>
VariablePtrType CrossEntropyLoss(VariablePtrType left, VariablePtrType right, SessionType &sess)
{
  // define the derivative
  std::function<void(VariablePtrType)> b_fn = [](VariablePtrType cur_node) {
    fetch::ml::ops::derivatives::CrossEntropyLoss(cur_node);
  };

  // define the forward function (i.e. the dot)
  std::function<void(VariablePtrType)> f_fn = [](VariablePtrType cur_node) {
    CELImplementation(cur_node);
  };

  // define the return variable with the Dot computation
  VariablePtrType ret = sess.Variable({1, 1}, "CEL", f_fn, b_fn, false);

  ret->prev.push_back(left);
  ret->prev.push_back(right);

  return ret;
}

template <typename VariablePtrType>
void SigmoidImplementation(VariablePtrType cur_node)
{
  cur_node->data() = fetch::math::Sigmoid(cur_node->prev[0]->data());
}
template <typename VariablePtrType, typename SessionType>
VariablePtrType Sigmoid(VariablePtrType left, SessionType &sess)
{
  // define the derivative
  std::function<void(VariablePtrType)> b_fn = [](VariablePtrType cur_node) {
    fetch::ml::ops::derivatives::Sigmoid(cur_node);
  };

  // define the forward function (i.e. the dot)
  std::function<void(VariablePtrType)> f_fn = [](VariablePtrType cur_node) {
    SigmoidImplementation(cur_node);
  };

  // define the return variable with the Dot computation
  VariablePtrType ret = sess.Variable(left->shape(), "Sigmoid", f_fn, b_fn, false);

  ret->prev.push_back(left);

  return ret;
}

};  // namespace ops
};  // namespace ml
};  // namespace fetch
