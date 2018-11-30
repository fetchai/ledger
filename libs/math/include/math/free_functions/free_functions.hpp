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

#include "math/kernels/approx_exp.hpp"
#include "math/kernels/approx_log.hpp"
#include "math/kernels/approx_logistic.hpp"
#include "math/kernels/approx_soft_max.hpp"
#include "math/kernels/basic_arithmetics.hpp"
#include "math/kernels/relu.hpp"
#include "math/kernels/sign.hpp"
#include "math/kernels/standard_functions.hpp"

#include "core/assert.hpp"
#include "math/ndarray_broadcast.hpp"
#include "meta/type_traits.hpp"
#include "vectorise/memory/range.hpp"
#include <algorithm>
#include <cassert>
#include <numeric>
#include <vector>

/////////////////////////////////
/// include specific math functions
/////////////////////////////////

#include "math/free_functions/fundamental_operators.hpp"  // add, subtract etc.
#include "math/free_functions/standard_functions/abs.hpp"
#include "math/free_functions/standard_functions/exp.hpp"
#include "math/free_functions/standard_functions/fmod.hpp"
#include "math/free_functions/standard_functions/log.hpp"
#include "math/free_functions/standard_functions/remainder.hpp"
#include "math/free_functions/statistics/normal.hpp"

/////////////////////////////////
/// blas libraries
/////////////////////////////////

#include "math/linalg/blas/gemm_nn_vector.hpp"
#include "math/linalg/blas/gemm_nn_vector_threaded.hpp"
#include "math/linalg/blas/gemm_nt_vector.hpp"
#include "math/linalg/blas/gemm_nt_vector_threaded.hpp"
#include "math/linalg/blas/gemm_tn_vector.hpp"
#include "math/linalg/blas/gemm_tn_vector_threaded.hpp"

#include "math/meta/type_traits.hpp"

#include "math/free_functions/exponentiation/exponentiation.hpp"
#include "math/free_functions/trigonometry/trigonometry.hpp"
#include "math/free_functions/comparison/comparison.hpp"
#include "math/free_functions/statistics/distributions.hpp"
#include "math/free_functions/precision/precision.hpp"
#include "math/free_functions/iteration/iteration.hpp"
#include "math/free_functions/deep_learning/activation_functions.hpp"
#include "math/free_functions/deep_learning/loss_functions.hpp"
#include "math/free_functions/matrix_operations/matrix_operations.hpp"
#include "math/free_functions/sign/sign.cpp"

namespace fetch {
namespace math {

template <typename T, typename C>
class ShapeLessArray;
template <typename T, typename C>
class NDArray;
template <typename T, typename C>
class NDArrayIterator;

namespace linalg {
template <typename T, typename C, typename S>
class Matrix;
}

template <typename T, typename C>
T Max(ShapeLessArray<T, C> const &array);

/**
 * calculates bit mask on this
 * @param x
 */
namespace details {
template <typename ArrayType>
void BooleanMaskImplementation(ArrayType &input_array, ArrayType const &mask, ArrayType &ret)
{
  assert(input_array.size() == mask.size());

  std::size_t counter = 0;
  for (std::size_t i = 0; i < input_array.size(); ++i)
  {
    assert((mask[i] == 1) || (mask[i] == 0));
    // TODO(private issue 193): implement boolean only ndarray to avoid cast
    if (bool(mask[i]))
    {
      ret[counter] = input_array[i];
      ++counter;
    }
  }

  ret.LazyResize(counter);
}
}  // namespace details
template <typename T, typename C>
void BooleanMask(ShapeLessArray<T, C> &input_array, ShapeLessArray<T, C> const &mask,
                 ShapeLessArray<T, C> &ret)
{
  details::BooleanMaskImplementation(input_array, mask, ret);
}
template <typename T, typename C>
ShapeLessArray<T, C> BooleanMask(ShapeLessArray<T, C> &      input_array,
                                 ShapeLessArray<T, C> const &mask)
{
  ShapeLessArray<T, C> ret;
  BooleanMask(input_array, mask, ret);
  return ret;
}
template <typename T, typename C>
void BooleanMask(NDArray<T, C> &input_array, NDArray<T, C> &mask, NDArray<T, C> &ret)
{
  assert(input_array.shape().size() >= mask.shape().size());
  assert(mask.shape().size() > 0);

  // because tensorflow is row major by default - we have to flip the mask and array to get the same
  // answer
  // TODO(private issue 208)
  input_array.MajorOrderFlip();
  mask.MajorOrderFlip();

  if (mask.shape() == input_array.shape())
  {
    details::BooleanMaskImplementation(input_array, mask, ret);
  }
  else
  {
    for (std::size_t j = 0; j < mask.shape().size(); ++j)
    {
      assert(mask.shape()[j] == input_array.shape()[j]);
    }

    // new shape should be n-k+1 dimensions
    std::vector<std::size_t> new_shape;
    NDArray<T, C>            ret{new_shape};

    // TODO(private issue 207): perhaps a little bit hacky to implement boolean mask as a
    // multiplication
    Broadcast([](T x, T y) { return x * y; }, input_array, mask, ret);
  }
}
template <typename T, typename C>
NDArray<T, C> BooleanMask(NDArray<T, C> &input_array, NDArray<T, C> &mask)
{
  NDArray<T, C> ret;
  BooleanMask(input_array, mask, ret);
  return ret;
}

/**
 * round to nearest int in float format
 * @param x
 */
template <typename ArrayType>
void Nearbyint(ArrayType &x)
{
  kernels::stdlib::Nearbyint<typename ArrayType::Type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 * finite check
 * @param x
 */
template <typename ArrayType>
void Isfinite(ArrayType &x)
{
  kernels::stdlib::Isfinite<typename ArrayType::Type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 * checks for inf values
 * @param x
 */
template <typename ArrayType>
void Isinf(ArrayType &x)
{
  kernels::stdlib::Isinf<typename ArrayType::Type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 * checks for nans
 * @param x
 */
template <typename ArrayType>
void Isnan(ArrayType &x)
{
  kernels::stdlib::Isnan<typename ArrayType::Type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 * If no errors occur and there are two inputs, the hypotenuse of a right-angled triangle is
 * computed as sqrt(x^2 + y^2) If no errors occur and there are 3 points, then the distance from the
 * origin in 3D space is returned as sqrt(x^2 + y^2 + z^2)
 * @param x
 */
template <typename ArrayType>
void Hypot(ArrayType &x)
{
  kernels::stdlib::Hypot<typename ArrayType::Type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 * Decomposes given floating point value arg into a normalized fraction and an integral power of
 * two.
 * @param x
 */
template <typename ArrayType>
fetch::math::meta::IsNotImplementedLike<ArrayType, void> Frexp(ArrayType &x)
{
  kernels::stdlib::Frexp<typename ArrayType::Type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 * Multiplies a floating point value x by the number 2 raised to the exp power.
 * @param x
 */
template <typename ArrayType>
fetch::math::meta::IsNotImplementedLike<ArrayType, void> Ldexp(ArrayType &x)
{
  kernels::stdlib::Ldexp<typename ArrayType::Type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 * Decomposes given floating point value x into integral and fractional parts, each having the same
 * type and sign as x. The integral part (in floating-point format) is stored in the object pointed
 * to by iptr.
 * @param x
 */
template <typename ArrayType>
fetch::math::meta::IsNotImplementedLike<ArrayType, void> Modf(ArrayType &x)
{
  kernels::stdlib::Modf<typename ArrayType::Type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 * Multiplies a floating point value x by FLT_RADIX raised to power exp.
 * @param x
 */
template <typename ArrayType>
fetch::math::meta::IsNotImplementedLike<ArrayType, void> Scalbn(ArrayType &x)
{
  kernels::stdlib::Scalbn<typename ArrayType::Type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 * Multiplies a floating point value x by FLT_RADIX raised to power exp.
 * @param x
 */
template <typename ArrayType>
fetch::math::meta::IsNotImplementedLike<ArrayType, void> Scalbln(ArrayType &x)
{
  kernels::stdlib::Scalbln<typename ArrayType::Type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 * Extracts the value of the unbiased exponent from the floating-point argument arg, and returns it
 * as a signed integer value.
 * @param x
 */
template <typename ArrayType>
fetch::math::meta::IsNotImplementedLike<ArrayType, void> Ilogb(ArrayType &x)
{
  kernels::stdlib::Ilogb<typename ArrayType::Type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 * Extracts the value of the unbiased radix-independent exponent from the floating-point argument
 * arg, and returns it as a floating-point value.
 * @param x
 */
template <typename ArrayType>
fetch::math::meta::IsNotImplementedLike<ArrayType, void> Logb(ArrayType &x)
{
  kernels::stdlib::Logb<typename ArrayType::Type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 * Categorizes floating point value arg into the following categories: zero, subnormal, normal,
 * infinite, NAN, or implementation-defined category.
 * @param x
 */
template <typename ArrayType>
fetch::math::meta::IsNotImplementedLike<ArrayType, void> Fpclassify(ArrayType &x)
{
  kernels::stdlib::Fpclassify<typename ArrayType::Type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 * Determines if the given floating point number arg is normal, i.e. is neither zero, subnormal,
 * infinite, nor NaN.
 * @param x
 */
template <typename ArrayType>
fetch::math::meta::IsNotImplementedLike<ArrayType, void> Isnormal(ArrayType &x)
{
  kernels::stdlib::Isnormal<typename ArrayType::Type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 * Determines if the floating point numbers x and y are unordered, that is, one or both are NaN and
 * thus cannot be meaningfully compared with each other.
 * @param x
 */
template <typename ArrayType>
fetch::math::meta::IsNotImplementedLike<ArrayType, void> Isunordered(ArrayType &x)
{
  kernels::stdlib::Isunordered<typename ArrayType::Type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

}  // namespace math
}  // namespace fetch
