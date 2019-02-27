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

#include "math/kernels/approx_logistic.hpp"
#include "math/kernels/basic_arithmetics.hpp"
#include "math/kernels/relu.hpp"
#include "math/kernels/standard_functions.hpp"

#include "core/assert.hpp"
#include "math/meta/math_type_traits.hpp"
#include "vectorise/memory/range.hpp"
#include <algorithm>
#include <cassert>
#include <numeric>
#include <vector>

#include "math/free_functions/fundamental_operators.hpp"  // add, subtract etc.
#include "math/free_functions/standard_functions/abs.hpp"
#include "math/free_functions/standard_functions/exp.hpp"
#include "math/free_functions/standard_functions/fmod.hpp"
#include "math/free_functions/standard_functions/remainder.hpp"

#include "math/free_functions/comparison/comparison.hpp"
#include "math/free_functions/deep_learning/loss_functions.hpp"
#include "math/free_functions/exponentiation/exponentiation.hpp"
#include "math/free_functions/matrix_operations/matrix_operations.hpp"
#include "math/meta/math_type_traits.hpp"

namespace fetch {
namespace math {

template <typename T, typename C>
class ShapelessArray;
template <typename T, typename C>
class NDArray;
template <typename T, typename C>
class NDArrayIterator;

template <typename T, typename C>
T Max(ShapelessArray<T, C> const &array);

/**
 *
 * @param x
 */
template <typename ArrayType>
void ApproxLogistic(ArrayType &x)
{
  kernels::ApproxLogistic<typename ArrayType::vector_register_type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 * rectified linear activation function
 * @param x
 */
template <typename ArrayType>
void Relu(ArrayType &x)
{
  kernels::Relu<typename ArrayType::vector_register_type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 * The sigmoid function
 * @tparam ArrayType
 * @tparam T
 * @param y
 * @param y_hat
 * @param ret
 */
template <typename ArrayType>
ArrayType Sigmoid(ArrayType const &A)
{
  ArrayType ret{A.shape()};
  ret.Copy(A);

  Multiply(typename ArrayType::Type(-1.0), ret, ret);
  Exp(ret);
  Add(ret, typename ArrayType::Type(1.0), ret);
  Divide(typename ArrayType::Type(1.0), ret, ret);

  return ret;
}

/**
 * softmax over all data in shapelessarray
 * @tparam T type
 * @tparam C container_type
 * @param array original data upon which to call softmax
 * @param ret new data with softmax applied
 */
namespace details {
template <typename ArrayType>
void SoftmaxImplementation(ArrayType const &array, ArrayType &ret)
{
  assert(ret.size() == array.size());

  // by subtracting the max we improve numerical stability, and the result will be identical
  std::vector<std::size_t> arr_shape{array.shape()[0], 1};
  ArrayType                array_max{arr_shape};
  ArrayType                array_sum{arr_shape};

  Max(array, 1, array_max);
  Subtract(array, array_max, ret);
  Exp(ret);

  ReduceSum(ret, 1, array_sum);
  Divide(ret, array_sum, ret);
}
}  // namespace details
template <typename T, typename C>
void Softmax(ShapelessArray<T, C> &array, ShapelessArray<T, C> &ret)
{
  assert(ret.size() == array.size());
  details::SoftmaxImplementation(array, ret);
}
template <typename T, typename C>
ShapelessArray<T, C> Softmax(ShapelessArray<T, C> &array)
{
  ShapelessArray<T, C> ret{array.size()};
  Softmax(array, ret);
  return ret;
}
template <typename T, typename C>
void Softmax(NDArray<T, C> const &array, NDArray<T, C> &ret)
{
  assert(ret.size() == array.size());
  ret.LazyReshape(array.shape());
  details::SoftmaxImplementation(array, ret);
}
template <typename T, typename C>
NDArray<T, C> Softmax(NDArray<T, C> const &array)
{
  NDArray<T, C> ret{array.shape()};
  Softmax(array, ret);
  return ret;
}

}  // namespace math
}  // namespace fetch
