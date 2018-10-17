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
#include "ml/ops/derivatives/activation_functions.hpp"

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
 * The Sigmoid activation squashes such that y = 1 / 1 + e^(-x)
 * @tparam VariablePtrType
 * @param cur_node
 */
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
  bool            is_leaf       = false;
  bool            requires_grad = false;
  VariablePtrType ret = sess.Variable(left->shape(), "Sigmoid", f_fn, b_fn, is_leaf, requires_grad);

  ret->prev.push_back(left);

  return ret;
}

/**
 * The Softmax gives some non-zero value to all outputs with e^(x) / (Sum(e^x))
 * @tparam VariablePtrType
 * @param cur_node
 */
template <typename VariablePtrType>
void SoftmaxImplementation(VariablePtrType cur_node)
{
  cur_node->data() = fetch::math::Softmax(cur_node->prev[0]->data());
}
template <typename VariablePtrType, typename SessionType>
VariablePtrType Softmax(VariablePtrType left, SessionType &sess)
{
  // define the derivative
  std::function<void(VariablePtrType)> b_fn = [](VariablePtrType cur_node) {
    fetch::ml::ops::derivatives::Softmax(cur_node);
  };

  // define the forward function (i.e. the dot)
  std::function<void(VariablePtrType)> f_fn = [](VariablePtrType cur_node) {
    SoftmaxImplementation(cur_node);
  };

  // define the return variable with the Dot computation
  bool            is_leaf       = false;
  bool            requires_grad = false;
  VariablePtrType ret = sess.Variable(left->shape(), "Softmax", f_fn, b_fn, is_leaf, requires_grad);

  ret->prev.push_back(left);

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
  assert(cur_node->prev.size() == 2);
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
  bool            is_leaf       = false;
  bool            requires_grad = false;
  VariablePtrType ret = sess.Variable(left->shape(), "Relu", f_fn, b_fn, is_leaf, requires_grad);

  ret->prev.push_back(left);
  ret->prev.push_back(zeros);

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
void LeakyReluImplementation(VariablePtrType cur_node)
{
  assert(cur_node->prev.size() == 2);
  // we assume that prev[1] hold the Variable full of zeros that was previously defined
  auto temp = fetch::math::Maximum(cur_node->prev[0]->data(), cur_node->prev[1]->data());

  for (std::size_t i = 0; i < temp.size(); ++i)
  {
    if (cur_node->prev[0]->data()[i] > 0)
    {
      cur_node->data()[i] = cur_node->prev[0]->data()[i];
    }
    else
    {
      cur_node->data()[i] = fetch::math::Multiply(0.01, cur_node->prev[0]->data()[i]);
    }
  }
}
template <typename VariablePtrType, typename SessionType>
VariablePtrType LeakyRelu(VariablePtrType left, SessionType &sess)
{
  // define the derivative
  std::function<void(VariablePtrType)> b_fn = [](VariablePtrType cur_node) {
    fetch::ml::ops::derivatives::LeakyRelu(cur_node);
  };

  // define the forward function (i.e. the dot)
  std::function<void(VariablePtrType)> f_fn = [](VariablePtrType cur_node) {
    LeakyReluImplementation(cur_node);
  };

  // define Variable of zeros to compare against
  VariablePtrType zeros = SessionType::Zeroes(left->shape(), sess);

  // define the return variable with the Relu computation
  bool            is_leaf       = false;
  bool            requires_grad = false;
  VariablePtrType ret =
      sess.Variable(left->shape(), "LeakyRelu", f_fn, b_fn, is_leaf, requires_grad);

  ret->prev.push_back(left);
  ret->prev.push_back(zeros);

  return ret;
}

}  // namespace ops
}  // namespace ml
}  // namespace fetch
