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

//#include "math/kernels/approx_exp.hpp"
//#include "math/kernels/approx_log.hpp"
//#include "math/kernels/approx_logistic.hpp"
//#include "math/kernels/approx_soft_max.hpp"
//#include "math/kernels/basic_arithmetics.hpp"
//#include "math/kernels/relu.hpp"
//#include "math/kernels/sign.hpp"
//#include "math/kernels/standard_functions.hpp"

#include "math/free_functions/free_functions.hpp"
//
//#include "core/assert.hpp"
//#include "core/meta/type_traits.hpp"
//#include "math/ndarray_broadcast.hpp"
//#include "vectorise/memory/range.hpp"
//#include <algorithm>
//#include <cassert>
//#include <numeric>
//#include <vector>

namespace fetch {
namespace math {
namespace ops {

template <typename T, typename C>
class ShapeLessArray;
template <typename T, typename C>
class NDArray;
template <typename T, typename C>
class NDArrayIterator;

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
void Sigmoid(ARRAY_TYPE const &y, ARRAY_TYPE &ret)
{
  Multiply(y, 1.0, ret);
  Exp(ret);
  Add(ret, 1.0, ret);

  Divide(1.0, ret, ret);

  //  ARRAY_TYPE temp(y.size());
  //  fetch::math::Subtract(y, y_hat, temp);
  //  fetch::math::Square(temp);
  //  fetch::math::Mean(temp, ret);
  //
  //  z = x * -1.0
  //  z.Exp()
  //  y = 1.0 / (z + 1.0)
  //
  //
}

/**
 * The rectified linear unit returns the elementwise maximum of 0 and y
 * @tparam ARRAY_TYPE
 * @tparam T
 * @param y
 * @param ret
 */
template <typename ARRAY_TYPE, typename T>
void Relu(ARRAY_TYPE const &y, ARRAY_TYPE &ret)
{
  ARRAY_TYPE zeros = ARRAY_TYPE::Zeros(y.height(), y.width());
  ret.Maximum(y, zeros, ret);
}

};  // namespace ops
};  // namespace math
};  // namespace fetch
