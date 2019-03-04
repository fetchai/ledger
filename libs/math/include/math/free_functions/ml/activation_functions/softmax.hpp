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
//#include "math/free_functions/deep_learning/loss_functions.hpp"
#include "math/free_functions/exponentiation/exponentiation.hpp"
#include "math/free_functions/matrix_operations/matrix_operations.hpp"
#include "math/meta/math_type_traits.hpp"

namespace fetch {
namespace math {

/**
 * softmax over all data in shapelessarray
 * @tparam T type
 * @tparam C container_type
 * @param array original data upon which to call softmax
 * @param ret new data with softmax applied
 */
namespace details {

/*
 * Really naive implementation that relies only on ArrayType providing a At(std::size_t) method
 * TODO(private, 520) -- Clean up once we get unified ArrayType + operations
 */

template <typename ArrayType>
void SoftmaxImplementation(ArrayType const &array, ArrayType &ret)
{
  assert(ret.size() == array.size());

  // by subtracting the max we improve numerical stability, and the result will be identical
  std::vector<typename ArrayType::SizeType> arr_shape{array.shape()[0], 1};
  ArrayType                                 array_max{arr_shape};
  ArrayType                                 array_sum{arr_shape};

  if (array.shape().size() == 1)
  {
    Max(array, 0, array_max);
  }
  else
  {
    Max(array, 1, array_max);
  }

  Subtract(array, array_max[0], ret);

  Exp(ret);

  if (ret.shape().size() == 1)
  {
    Sum(ret, array_sum[0]);
  }
  else
  {
    ReduceSum(ret, 1, array_sum);
  }

  for (std::size_t j = 0; j < ret.size(); ++j)
  {
    Divide(ret[j], array_sum[0], ret[j]);
  }
}
}  // namespace details

template <typename ArrayType>
void Softmax(ArrayType const &array, ArrayType &ret)
{
  assert(ret.size() == array.size());
  details::SoftmaxImplementation(array, ret);
}
template <typename ArrayType>
ArrayType Softmax(ArrayType const &array)
{
  ArrayType ret{array.size()};
  Softmax(array, ret);
  return ret;
}

}  // namespace math
}  // namespace fetch
