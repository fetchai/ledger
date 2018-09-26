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
#include "ml/variable.hpp"

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
template <typename T>
Variable<T> Dot(Variable<T> &left, Variable<T> &right)
{
  // define the derivative
  std::function<void(Variable<T> &)> b_fn = [](Variable<T> &cur_node) {
    fetch::ml::ops::derivatives::Dot(cur_node);
  };

  // define the return variable with the Dot computation
  Variable<T> ret = Variable<T>(fetch::math::Dot(left.data, right.data), b_fn, false);

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
template <typename T>
Variable<T> Relu(Variable<T> &left)
{
  // define the derivative
  std::function<void(Variable<T> &)> b_fn = [](Variable<T> &cur_node) {
    fetch::ml::ops::derivatives::Relu(cur_node);
  };

  // set up the new return node and calculate Relu
  Variable<T> ret{fetch::math::Maximum(left.data, T::Zeros(left.data.shape())), b_fn, false};
  ret.prev.push_back(left);

  return ret;
}

/**
 *
 * @return
 */
template <typename T>
Variable<T> Sum(Variable<T> &left)
{
  // define the derivative
  std::function<void(Variable<T> &)> b_fn = [](Variable<T> &cur_node) {
    fetch::ml::ops::derivatives::Sum(cur_node);
  };

  // set up the return node and calculate sum
  T sum_ret{1};
  sum_ret[0] = fetch::math::Sum(left.data);
  Variable<T> ret{sum_ret, b_fn, false};
  ret.prev.push_back(left);
  return ret;
}

template <typename ARRAY_TYPE, typename T>
void MeanSquareError(ARRAY_TYPE y, ARRAY_TYPE y_hat, T &ret)
{
  ARRAY_TYPE temp(y.size());
  fetch::math::Subtract(y, y_hat, temp);
  fetch::math::Square(temp);
  fetch::math::Mean(temp, ret);
}

/**
 * The sigmoid function
 * @tparam ARRAY_TYPE
 * @tparam T
 * @param y
 * @param y_hat
 * @param ret
 */
template <typename ARRAY_TYPE>
void Sigmoid(ARRAY_TYPE &y, ARRAY_TYPE &ret)
{
  Multiply(y, 1.0, ret);
  Exp(ret);
  Add(ret, 1.0, ret);

  Divide(1.0, ret, ret);
}

};  // namespace ops
};  // namespace ml
};  // namespace fetch
