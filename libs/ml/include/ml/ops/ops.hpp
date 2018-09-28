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
template <typename LayerType, typename SessionType>
LayerType Dot(LayerType &left, LayerType &right, SessionType &sess)
{
  // define the derivative
  std::function<void(LayerType &)> b_fn = [](LayerType &cur_node) {
    fetch::ml::ops::derivatives::Dot(cur_node);
  };

  // define the return variable with the Dot computation
  LayerType ret{sess};
  ret.Initialise(fetch::math::Dot(left.data, right.data), b_fn, false);
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
template <typename LayerType, typename SessionType>
LayerType Relu(LayerType &left, SessionType &sess)
{
  // define the derivative
  std::function<void(LayerType &)> b_fn = [](LayerType &cur_node) {
    fetch::ml::ops::derivatives::Relu(cur_node);
  };

  // set up the new return node and calculate Relu
  LayerType ret{sess};
  ret.Initialise(fetch::math::Maximum(left.data, LayerType::Zeroes(left.data.shape(), sess).data),
                 b_fn, false);
  //  ret.Initialise(fetch::math::Maximum(left.data, LayerType::type::Zeroes(left.data.shape(),
  //  sess)), b_fn, false);

  ret.prev.push_back(left);

  return ret;
}

/**
 *
 * @return
 */
template <typename LayerType, typename SessionType>
LayerType Sum(LayerType &left, std::size_t const axis, SessionType &sess)
{
  // define the derivative
  std::function<void(LayerType &)> b_fn = [](LayerType &cur_node) {
    fetch::ml::ops::derivatives::Sum(cur_node);
  };

  LayerType ret{sess};
  ret.Initialise(fetch::math::ReduceSum(left.data, axis), b_fn, false);

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
