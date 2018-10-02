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
template <typename VariableType, typename SessionType>
VariableType Dot(VariableType &left, VariableType &right, SessionType &sess)
{
  // define the derivative
  std::function<void(VariableType &)> b_fn = [](VariableType &cur_node) {
    fetch::ml::ops::derivatives::Dot(cur_node);
  };

  // define the return variable with the Dot computation
  VariableType ret{sess, fetch::math::Dot(left.data(), right.data()), "Dot", b_fn, false};
  ret.prev.push_back(left);
  ret.prev.push_back(right);

  return ret;
}

/**
 * The rectified linear unit returns the elementwise maximum of 0 and y
 * @tparam ARRAY_TYPE
 * @tparam T
 * @param y
 * @param ret
 */
template <typename VariableType, typename SessionType>
VariableType Relu(VariableType const &left, SessionType &sess)
{
  // define the derivative
  std::function<void(VariableType &)> b_fn = [](VariableType &cur_node) {
    fetch::ml::ops::derivatives::Relu(cur_node);
  };

  // set up the new return node and calculate Relu
  VariableType ret{
      sess,
      fetch::math::Maximum(left.data(), VariableType::Zeroes(left.data().shape(), sess).data()),
      "Relu",
      b_fn,
      false};

  ret.prev.push_back(left);

  return ret;
}

/**
 *
 * @return
 */
template <typename VariableType, typename SessionType>
VariableType Sum(VariableType &left, std::size_t const axis, SessionType &sess)
{
  // define the derivative
  std::function<void(VariableType &)> b_fn = [](VariableType &cur_node) {
    fetch::ml::ops::derivatives::Sum(cur_node);
  };

  VariableType ret{sess, fetch::math::ReduceSum(left.data(), axis), "Sum", b_fn, false};

  ret.prev.push_back(left);
  return ret;
}

template <typename VariableType, typename SessionType>
VariableType MeanSquareError(VariableType &left, VariableType &right, SessionType &sess)
{
  // define the derivative
  std::function<void(VariableType &)> b_fn = [](VariableType &cur_node) {
    fetch::ml::ops::derivatives::MeanSquareError(cur_node);
  };

  // define the return variable with the Dot computation
  VariableType ret{sess, fetch::math::MeanSquareError(left.data(), right.data()), "MSE", b_fn, false};

  ret.prev.push_back(left);

  return ret;
}

template <typename VariableType, typename SessionType>
VariableType Sigmoid(VariableType &left, SessionType &sess)
{
  // define the derivative
  std::function<void(VariableType &)> b_fn = [](VariableType &cur_node) {
    fetch::ml::ops::derivatives::Sigmoid(cur_node);
  };

  // define the return variable with the Dot computation
  VariableType ret{sess, fetch::math::Sigmoid(left.data()), "Sigmoid", b_fn, false};

  ret.prev.push_back(left);

  return ret;
}

template <typename VariableType, typename SessionType>
VariableType CrossEntropyLoss(VariableType &left, VariableType &right, SessionType &sess)
{
  // define the derivative
  std::function<void(VariableType &)> b_fn = [](VariableType &cur_node) {
    fetch::ml::ops::derivatives::CrossEntropyLoss(cur_node);
  };

  // define the return variable with the Dot computation
  VariableType ret{sess, fetch::math::CrossEntropyLoss(left.data(), right.data()), "CEL", b_fn, false};

  ret.prev.push_back(left);

  return ret;
}

};  // namespace ops
};  // namespace ml
};  // namespace fetch
