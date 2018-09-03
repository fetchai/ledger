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
#include <cassert>
//#include "math/linalg/matrix.hpp"
//#include "math/ndarray.hpp"
//#include "math/ndarray_iterator.hpp"
//#include "math/shape_less_array.hpp"
#include "math/ndarray_broadcast.hpp"
#include "vectorise/memory/range.hpp"

#include <algorithm>
#include <vector>

namespace fetch {
namespace math {

template <typename T, typename C>
class ShapeLessArray;
template <typename T, typename C>
class NDArray;
template <typename T, typename C>
class NDArrayIterator;

/**
 * Copies the values of updates into the specified indices of the first dimension of data in this
 * object
 */
template <typename T, typename C>
void Scatter(NDArray<T, C> &input_array, std::vector<T> &updates, std::vector<std::size_t> &indices)
{
  // sort indices and updates into ascending order

  std::vector<std::pair<std::size_t, T>> AB;

  // copy into pairs
  // Note that A values are put in "first" this is very important
  for (std::size_t i = 0; i < updates.size(); ++i)
  {
    AB.push_back(std::make_pair(indices[i], updates[i]));
  }

  std::sort(AB.begin(), AB.end());

  // Place back into arrays
  for (size_t i = 0; i < updates.size(); ++i)
  {
    updates[i] = AB[i].second;  //<- This is actually optional
    indices[i] = AB[i].first;
  }

  //  assert(indices.back() <= input_array.shape()[0]);

  // set up an iterator
  NDArrayIterator<T, C> arr_iterator{input_array};

  // scatter
  std::size_t cur_idx, arr_count = 0;
  for (std::size_t count = 0; count < indices.size(); ++count)
  {
    cur_idx = indices[count];

    while (arr_count < cur_idx)
    {
      ++arr_iterator;
      ++arr_count;
    }

    *arr_iterator = updates[count];
  }
}

/**
 * gathers data from first dimension of data, according to indices, and puts them into input array
 * self_type
 */
template <typename T, typename C>
void Gather(NDArray<T, C> &input_array, NDArray<T, C> &updates, std::vector<std::size_t> &indices)
{

  assert(input_array.size() == updates.size());
  input_array.LazyReshape(updates.shape());

  // sort indices
  std::sort(indices.begin(), indices.end());

  // check largest value in indices < shape()[0]
  assert(indices.back() <= updates.shape()[0]);

  // set up an iterator
  NDArrayIterator<T, C> arr_iterator{updates};
  NDArrayIterator<T, C> ret_iterator{input_array};

  std::size_t cur_idx, arr_count = 0;
  for (std::size_t count = 0; count < indices.size(); ++count)
  {
    cur_idx = indices[count];

    while (arr_count < cur_idx)
    {
      ++arr_iterator;
      ++arr_count;
    }

    *ret_iterator = *arr_iterator;
  }
}

/**
 * interleave data from multiple sources
 * @param x
 */
template <typename T, typename C>
void DynamicStitch(ShapeLessArray<T, C> &                       input_array,
                   std::vector<std::vector<std::size_t>> const &indices,
                   std::vector<ShapeLessArray<T, C>> const &    data)
{
  // identify the new size of this
  std::size_t new_size = 0;
  for (std::size_t l = 0; l < indices.size(); ++l)
  {
    new_size += indices[l].size();
  }

  input_array.LazyResize(new_size);

  // loop through all output data locations identifying the next data point to copy into it
  for (std::size_t i = 0; i < indices.size(); ++i)  // iterate through lists of indices
  {
    for (std::size_t k = 0; k < indices[i].size(); ++k)  // iterate through index within this list
    {
      assert(indices[i][k] <= input_array.size());
      input_array[indices[i][k]] = data[i][k];
    }
  }
}
template <typename T, typename C>
void DynamicStitch(NDArray<T, C> &input_array, std::vector<std::vector<std::size_t>> const &indices,
                   std::vector<NDArray<T, C>> const &data)
{
  // identify the new size of this
  std::size_t new_size = 0;
  for (std::size_t l = 0; l < indices.size(); ++l)
  {
    new_size += indices[l].size();
  }

  input_array.LazyResize(new_size);

  // loop through all output data locations identifying the next data point to copy into it
  for (std::size_t i = 0; i < indices.size(); ++i)  // iterate through lists of indices
  {
    for (std::size_t k = 0; k < indices[i].size(); ++k)  // iterate through index within this list
    {
      assert(indices[i][k] <= input_array.size());
      input_array[indices[i][k]] = data[i][k];
    }
  }
}

/**
 * calculates bit mask on this
 * @param x
 */
namespace details {
template <typename ARRAY_TYPE>
void BooleanMaskImplementation(ARRAY_TYPE &input_array, ARRAY_TYPE const &mask, ARRAY_TYPE &ret)
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

    // need to use broadcasting here.
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
 * assigns the absolute of x to this array
 * @param x
 */
template <typename ARRAY_TYPE>
void Abs(ARRAY_TYPE &x)
{
  kernels::stdlib::Abs<typename ARRAY_TYPE::type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 * e^x
 * @param x
 */
template <typename ARRAY_TYPE>
void Exp(ARRAY_TYPE &x)
{
  kernels::stdlib::Exp<typename ARRAY_TYPE::type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 * raise 2 to power input values of x
 * @param x
 */
template <typename ARRAY_TYPE>
void Exp2(ARRAY_TYPE &x)
{
  kernels::stdlib::Exp2<typename ARRAY_TYPE::type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 * exp(x) - 1
 * @param x
 */
template <typename ARRAY_TYPE>
void Expm1(ARRAY_TYPE &x)
{
  kernels::stdlib::Expm1<typename ARRAY_TYPE::type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 * natural logarithm of x
 * @param x
 */
template <typename ARRAY_TYPE>
void Log(ARRAY_TYPE &x)
{
  kernels::stdlib::Log<typename ARRAY_TYPE::type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 * natural logarithm of x
 * @param x
 */
template <typename ARRAY_TYPE>
void Log10(ARRAY_TYPE &x)
{
  kernels::stdlib::Log10<typename ARRAY_TYPE::type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 * log base 2
 * @param x
 */
template <typename ARRAY_TYPE>
void Log2(ARRAY_TYPE &x)
{
  kernels::stdlib::Log2<typename ARRAY_TYPE::type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 * natural log 1 + x
 * @param x
 */
template <typename ARRAY_TYPE>
void Log1p(ARRAY_TYPE &x)
{
  kernels::stdlib::Log1p<typename ARRAY_TYPE::type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 * square root
 * @param x
 */
template <typename ARRAY_TYPE>
void Sqrt(ARRAY_TYPE &x)
{
  kernels::stdlib::Sqrt<typename ARRAY_TYPE::type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 * cubic root x
 * @param x
 */
template <typename ARRAY_TYPE>
void Cbrt(ARRAY_TYPE &x)
{
  kernels::stdlib::Cbrt<typename ARRAY_TYPE::type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 * raise to power
 * @param x
 */
template <typename ARRAY_TYPE>
void Pow(ARRAY_TYPE &x)
{
  kernels::stdlib::Pow<typename ARRAY_TYPE::type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 * sine of x
 * @param x
 */
template <typename ARRAY_TYPE>
void Sin(ARRAY_TYPE &x)
{
  kernels::stdlib::Sin<typename ARRAY_TYPE::type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 * cosine of x
 * @param x
 */
template <typename ARRAY_TYPE>
void Cos(ARRAY_TYPE &x)
{
  kernels::stdlib::Cos<typename ARRAY_TYPE::type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 * tangent of x
 * @param x
 */
template <typename ARRAY_TYPE>
void Tan(ARRAY_TYPE &x)
{
  kernels::stdlib::Tan<typename ARRAY_TYPE::type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 * arc sine of x
 * @param x
 */
template <typename ARRAY_TYPE>
void Asin(ARRAY_TYPE &x)
{
  kernels::stdlib::Asin<typename ARRAY_TYPE::type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 * arc cosine of x
 * @param x
 */
template <typename ARRAY_TYPE>
void Acos(ARRAY_TYPE &x)
{
  kernels::stdlib::Acos<typename ARRAY_TYPE::type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 * arc tangent of x
 * @param x
 */
template <typename ARRAY_TYPE>
void Atan(ARRAY_TYPE &x)
{
  kernels::stdlib::Atan<typename ARRAY_TYPE::type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 * arc tangent of x
 * @param x
 */
template <typename ARRAY_TYPE>
void Atan2(ARRAY_TYPE &x)
{
  kernels::stdlib::Atan2<typename ARRAY_TYPE::type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 * hyperbolic sine of x
 * @param x
 */
template <typename ARRAY_TYPE>
void Sinh(ARRAY_TYPE &x)
{
  kernels::stdlib::Sinh<typename ARRAY_TYPE::type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 * hyperbolic cosine of x
 * @param x
 */
template <typename ARRAY_TYPE>
void Cosh(ARRAY_TYPE &x)
{
  kernels::stdlib::Cosh<typename ARRAY_TYPE::type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 * hyperbolic tangent of x
 * @param x
 */
template <typename ARRAY_TYPE>
void Tanh(ARRAY_TYPE &x)
{
  kernels::stdlib::Tanh<typename ARRAY_TYPE::type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 * hyperbolic arc sine of x
 * @param x
 */
template <typename ARRAY_TYPE>
void Asinh(ARRAY_TYPE &x)
{
  kernels::stdlib::Asinh<typename ARRAY_TYPE::type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 * hyperbolic arc cosine of x
 * @param x
 */
template <typename ARRAY_TYPE>
void Acosh(ARRAY_TYPE &x)
{
  kernels::stdlib::Acosh<typename ARRAY_TYPE::type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 * hyperbolic arc tangent of x
 * @param x
 */
template <typename ARRAY_TYPE>
void Atanh(ARRAY_TYPE &x)
{
  kernels::stdlib::Atanh<typename ARRAY_TYPE::type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 * error function of x
 * @param x
 */
template <typename ARRAY_TYPE>
void Erf(ARRAY_TYPE &x)
{
  kernels::stdlib::Erf<typename ARRAY_TYPE::type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 * complementary error function of x
 * @param x
 */
template <typename ARRAY_TYPE>
void Erfc(ARRAY_TYPE &x)
{
  kernels::stdlib::Erfc<typename ARRAY_TYPE::type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 * factorial of x-1
 * @param x
 */
template <typename ARRAY_TYPE>
void Tgamma(ARRAY_TYPE &x)
{
  kernels::stdlib::Tgamma<typename ARRAY_TYPE::type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 * log of factorial of x-1
 * @param x
 */
template <typename ARRAY_TYPE>
void Lgamma(ARRAY_TYPE &x)
{
  kernels::stdlib::Lgamma<typename ARRAY_TYPE::type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 * ceiling round
 * @param x
 */
template <typename ARRAY_TYPE>
void Ceil(ARRAY_TYPE &x)
{
  kernels::stdlib::Ceil<typename ARRAY_TYPE::type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 * floor rounding
 * @param x
 */
template <typename ARRAY_TYPE>
void Floor(ARRAY_TYPE &x)
{
  kernels::stdlib::Floor<typename ARRAY_TYPE::type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 * round towards 0
 * @param x
 */
template <typename ARRAY_TYPE>
void Trunc(ARRAY_TYPE &x)
{
  kernels::stdlib::Trunc<typename ARRAY_TYPE::type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 * round to nearest int in int format
 * @param x
 */
template <typename ARRAY_TYPE>
void Round(ARRAY_TYPE &x)
{
  kernels::stdlib::Round<typename ARRAY_TYPE::type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 * round to nearest int in float format
 * @param x
 */
template <typename ARRAY_TYPE>
void Lround(ARRAY_TYPE &x)
{
  kernels::stdlib::Lround<typename ARRAY_TYPE::type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 * round to nearest int in float format with long long return
 * @param x
 */
template <typename ARRAY_TYPE>
void Llround(ARRAY_TYPE &x)
{
  kernels::stdlib::Llround<typename ARRAY_TYPE::type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 * round to nearest int in float format
 * @param x
 */
template <typename ARRAY_TYPE>
void Nearbyint(ARRAY_TYPE &x)
{
  kernels::stdlib::Nearbyint<typename ARRAY_TYPE::type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 * round to nearest int
 * @param x
 */
template <typename ARRAY_TYPE>
void Rint(ARRAY_TYPE &x)
{
  kernels::stdlib::Rint<typename ARRAY_TYPE::type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 *
 * @param x
 */
template <typename ARRAY_TYPE>
void Lrint(ARRAY_TYPE &x)
{
  kernels::stdlib::Lrint<typename ARRAY_TYPE::type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 *
 * @param x
 */
template <typename ARRAY_TYPE>
void Llrint(ARRAY_TYPE &x)
{
  kernels::stdlib::Llrint<typename ARRAY_TYPE::type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 * finite check
 * @param x
 */
template <typename ARRAY_TYPE>
void Isfinite(ARRAY_TYPE &x)
{
  kernels::stdlib::Isfinite<typename ARRAY_TYPE::type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 * checks for inf values
 * @param x
 */
template <typename ARRAY_TYPE>
void Isinf(ARRAY_TYPE &x)
{
  kernels::stdlib::Isinf<typename ARRAY_TYPE::type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 * checks for nans
 * @param x
 */
template <typename ARRAY_TYPE>
void Isnan(ARRAY_TYPE &x)
{
  kernels::stdlib::Isnan<typename ARRAY_TYPE::type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 *
 * @param x
 */
template <typename ARRAY_TYPE>
void Hypot(ARRAY_TYPE &x)
{
  kernels::stdlib::Hypot<typename ARRAY_TYPE::type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 *
 * @param x
 */
template <typename ARRAY_TYPE>
void Frexp(ARRAY_TYPE &x)
{
  kernels::stdlib::Frexp<typename ARRAY_TYPE::type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 *
 * @param x
 */
template <typename ARRAY_TYPE>
void Ldexp(ARRAY_TYPE &x)
{
  kernels::stdlib::Ldexp<typename ARRAY_TYPE::type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 *
 * @param x
 */
template <typename ARRAY_TYPE>
void Modf(ARRAY_TYPE &x)
{
  kernels::stdlib::Modf<typename ARRAY_TYPE::type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 *
 * @param x
 */
template <typename ARRAY_TYPE>
void Scalbn(ARRAY_TYPE &x)
{
  kernels::stdlib::Scalbn<typename ARRAY_TYPE::type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 *
 * @param x
 */
template <typename ARRAY_TYPE>
void Scalbln(ARRAY_TYPE &x)
{
  kernels::stdlib::Scalbln<typename ARRAY_TYPE::type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 *
 * @param x
 */
template <typename ARRAY_TYPE>
void Ilogb(ARRAY_TYPE &x)
{
  kernels::stdlib::Ilogb<typename ARRAY_TYPE::type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 *
 * @param x
 */
template <typename ARRAY_TYPE>
void Logb(ARRAY_TYPE &x)
{
  kernels::stdlib::Logb<typename ARRAY_TYPE::type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 *
 * @param x
 */
template <typename ARRAY_TYPE>
void Nextafter(ARRAY_TYPE &x)
{
  kernels::stdlib::Nextafter<typename ARRAY_TYPE::type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 *
 * @param x
 */
template <typename ARRAY_TYPE>
void Nexttoward(ARRAY_TYPE &x)
{
  kernels::stdlib::Nexttoward<typename ARRAY_TYPE::type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 *
 * @param x
 */
template <typename ARRAY_TYPE>
void Copysign(ARRAY_TYPE &x)
{
  kernels::stdlib::Copysign<typename ARRAY_TYPE::type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 *
 * @param x
 */
template <typename ARRAY_TYPE>
void Fpclassify(ARRAY_TYPE &x)
{
  kernels::stdlib::Fpclassify<typename ARRAY_TYPE::type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 *
 * @param x
 */
template <typename ARRAY_TYPE>
void Isnormal(ARRAY_TYPE &x)
{
  kernels::stdlib::Isnormal<typename ARRAY_TYPE::type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 *
 * @param x
 */
template <typename ARRAY_TYPE>
void Signbit(ARRAY_TYPE &x)
{
  kernels::stdlib::Signbit<typename ARRAY_TYPE::type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 *
 * @param x
 */
template <typename ARRAY_TYPE>
void Isgreater(ARRAY_TYPE &x)
{
  kernels::stdlib::Isgreater<typename ARRAY_TYPE::type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 *
 * @param x
 */
template <typename ARRAY_TYPE>
void Isgreaterequal(ARRAY_TYPE &x)
{
  kernels::stdlib::Isgreaterequal<typename ARRAY_TYPE::type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 *
 * @param x
 */
template <typename ARRAY_TYPE>
void Isless(ARRAY_TYPE &x)
{
  kernels::stdlib::Isless<typename ARRAY_TYPE::type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 *
 * @param x
 */
template <typename ARRAY_TYPE>
void Islessequal(ARRAY_TYPE &x)
{
  kernels::stdlib::Islessequal<typename ARRAY_TYPE::type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 *
 * @param x
 */
template <typename ARRAY_TYPE>
void Islessgreater(ARRAY_TYPE &x)
{
  kernels::stdlib::Islessgreater<typename ARRAY_TYPE::type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 *
 * @param x
 */
template <typename ARRAY_TYPE>
void Isunordered(ARRAY_TYPE &x)
{
  kernels::stdlib::Isunordered<typename ARRAY_TYPE::type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 *
 * @param x
 */
template <typename ARRAY_TYPE>
void ApproxExp(ARRAY_TYPE &x)
{
  kernels::ApproxExp<typename ARRAY_TYPE::vector_register_type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 *
 * @param x
 */
template <typename ARRAY_TYPE>
void ApproxLog(ARRAY_TYPE &x)
{
  kernels::ApproxLog<typename ARRAY_TYPE::vector_register_type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 *
 * @param x
 */
template <typename ARRAY_TYPE>
void ApproxLogistic(ARRAY_TYPE &x)
{
  kernels::ApproxLogistic<typename ARRAY_TYPE::vector_register_type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 * rectified linear activation function
 * @param x
 */
template <typename ARRAY_TYPE>
void Relu(ARRAY_TYPE &x)
{
  kernels::Relu<typename ARRAY_TYPE::vector_register_type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 * replaces data with the sign (1 or -1)
 * @param x
 */
template <typename ARRAY_TYPE>
void Sign(ARRAY_TYPE &x)
{
  kernels::Sign<typename ARRAY_TYPE::vector_register_type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 * Max function for two values
 * @tparam T
 * @param datum1
 * @param datum2
 * @return
 */
template <typename T>
T &Max(T const &datum1, T const &datum2, T &ret)
{
  ret = std::max(datum1, datum2);
  return ret;
}

/**
 * Finds the maximum value in an array
 * @tparam T data type
 * @tparam C container type
 * @param array array to search for maximum value
 * @return
 */
template <typename T, typename C>
T &Max(ShapeLessArray<T, C> const &array, T &ret)
{
  using vector_register_type = typename ShapeLessArray<T, C>::vector_register_type;

  ret = array.data().in_parallel().Reduce(
      memory::TrivialRange(0, array.size()),
      [](vector_register_type const &a, vector_register_type const &b) { return max(a, b); });
  return ret;
}

/**
 * Finds the maximum value in a range of the array
 * @tparam T
 * @tparam C
 * @param array
 * @param r
 * @return
 */
template <typename T, typename C>
inline void Max(ShapeLessArray<T, C> const &array, memory::Range r, T &ret)
{
  using vector_register_type = typename ShapeLessArray<T, C>::vector_register_type;

  if (r.is_trivial())
  {
    ret = array.data().in_parallel().Reduce(
        r, [](vector_register_type const &a, vector_register_type const &b) { return max(a, b); });
  }
  else
  {  // non-trivial range is not vectorised
    typename ShapeLessArray<T, C>::type ret =
        -std::numeric_limits<typename ShapeLessArray<T, C>::type>::max();
    for (auto i : array)
    {
      ret = std::max(ret, i);
    }
  }
}

/**
 * Implementation of Max that returns the n-1 dim array by finding the max of all 1-d vectors within
 * the array
 * @tparam T
 * @tparam C
 * @param array
 * @param axis
 * @param ret
 */
template <typename T, typename C>
void Max(NDArray<T, C> &array, std::size_t const &axis, NDArray<T, C> &ret)
{
  assert(axis < array.shape().size());

  NDArrayIterator<typename NDArray<T, C>::type, typename NDArray<T, C>::container_type>
      return_iterator{ret};

  // iterate through the return array (i.e. the array of Max vals)
  std::vector<std::size_t> cur_index;
  while (return_iterator)
  {
    std::vector<std::vector<std::size_t>> cur_step;

    cur_index = return_iterator.GetNDimIndex();

    // calculate which part of the array we should iterate over (i.e. identify the 1-d vectors
    // within the array)
    std::size_t index_counter = 0;
    for (std::size_t i = 0; i < array.shape().size(); ++i)
    {
      if (i == axis)
      {
        cur_step.push_back({0, array.shape()[i]});
      }
      else
      {
        cur_step.push_back({cur_index[index_counter], cur_index[index_counter] + 1});
        ++index_counter;
      }
    }

    // get an iterator to iterate over the 1-d slice of the array to calculate max over
    NDArrayIterator<typename NDArray<T, C>::type, typename NDArray<T, C>::container_type>
        array_iterator(array, cur_step);

    // loops through the 1d array calculating the max val
    typename NDArray<T, C>::type cur_max =
        -std::numeric_limits<typename NDArray<T, C>::type>::max();
    typename NDArray<T, C>::type cur_val;
    while (array_iterator)
    {
      cur_val = *array_iterator;
      Max(cur_max, cur_val, cur_max);
      ++array_iterator;
    }

    *return_iterator = cur_max;
    ++return_iterator;
  }
}

/**
 * Min function for two values
 * @tparam T
 * @param datum1
 * @param datum2
 * @return
 */
template <typename T>
inline void Min(T const &datum1, T const &datum2, T &ret)
{
  ret = std::min(datum1, datum2);
}

/**
 * Min function for returning the smallest value in an array
 * @tparam ARRAY_TYPE
 * @param array
 * @return
 */
template <typename T, typename C>
inline void Min(ShapeLessArray<T, C> const &array, T &ret)
{
  using vector_register_type = typename ShapeLessArray<T, C>::vector_register_type;

  ret = array.data().in_parallel().Reduce(
      memory::TrivialRange(0, array.size()),
      [](vector_register_type const &a, vector_register_type const &b) { return min(a, b); });
}

/**
 * Finds the minimum value in a range of the array
 * @tparam T
 * @tparam C
 * @param array
 * @param r
 * @return
 */
template <typename T, typename C>
inline void Min(ShapeLessArray<T, C> const &array, memory::Range r, T &ret)
{
  using vector_register_type = typename ShapeLessArray<T, C>::vector_register_type;

  if (r.is_trivial())
  {
    ret = array.data().in_parallel().Reduce(
        r, [](vector_register_type const &a, vector_register_type const &b) { return min(a, b); });
  }
  else
  {  // non-trivial range is not vectorised
    typename T::type ret = std::numeric_limits<typename T::type>::max();
    for (auto i : array)
    {
      ret = std::min(ret, i);
    }
  }
}

/**
 * find the minimum of the 1-D projections through the array
 */
template <typename T, typename C>
void Min(NDArray<T, C> &array, std::size_t const &axis, NDArray<T, C> &ret)
{
  assert(axis < array.shape().size());

  NDArrayIterator<T, C> return_iterator{ret};

  // iterate through the return array (i.e. the array of Max vals)
  //    type cur_val;
  std::vector<std::size_t> cur_index;
  while (return_iterator)
  {
    std::vector<std::vector<std::size_t>> cur_step;

    cur_index = return_iterator.GetNDimIndex();

    // calculate step from cur_index and axis
    std::size_t index_counter = 0;
    for (std::size_t i = 0; i < array.shape().size(); ++i)
    {
      if (i == axis)
      {
        cur_step.push_back({0, array.shape()[i]});
      }
      else
      {
        cur_step.push_back({cur_index[index_counter], cur_index[index_counter] + 1});
        ++index_counter;
      }
    }

    // get an iterator to iterate over the 1-d slice of the array to calculate max over
    NDArrayIterator<T, C> array_iterator(array, cur_step);

    // loops through the 1d array calculating the max val
    T cur_max = std::numeric_limits<T>::max();
    T cur_val;
    while (array_iterator)
    {
      cur_val = *array_iterator;
      Min(cur_max, cur_val, cur_max);
      ++array_iterator;
    }

    *return_iterator = cur_max;
    ++return_iterator;
  }
}

/**
 * softmax over all data in shapelessarray
 * @tparam T type
 * @tparam C container_type
 * @param array original data upon which to call softmax
 * @param ret new data with softmax applied
 */
namespace details {
template <typename ARRAY_TYPE>
void SoftmaxImplementation(ARRAY_TYPE const &array, ARRAY_TYPE &ret)
{
  ret.LazyResize(array.size());

  // by subtracting the max we improve numerical stability, and the result will be identical
  typename ARRAY_TYPE::type array_max, array_sum;
  Max(array, array_max);
  Subtract(array, array_max, ret);
  Exp(ret);
  Sum(ret, array_sum);
  Divide(ret, array_sum, ret);
}
}  // namespace details
template <typename T, typename C>
void Softmax(ShapeLessArray<T, C> const &array, ShapeLessArray<T, C> &ret)
{
  details::SoftmaxImplementation(array, ret);
}
template <typename T, typename C>
ShapeLessArray<T, C> Softmax(ShapeLessArray<T, C> const &array)
{
  ShapeLessArray<T, C> ret;
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
  NDArray<T, C> ret;
  Softmax(array, ret);
  return ret;
}

/**
 * Returns an array containing the elementwise maximum of two other ndarrays
 * @param x array input 1
 * @param y array input 2
 * @return the combined array
 */
namespace details {
template <typename ARRAY_TYPE>
ARRAY_TYPE &MaximumImplementation(ARRAY_TYPE const &array1, ARRAY_TYPE const &array2,
                                  ARRAY_TYPE &ret)
{
  assert(array1.size() == array2.size());
  ret.Resize(array1.size());

  for (std::size_t i = 0; i < ret.size(); ++i)
  {
    ret[i] = std::max(array1[i], array2[i]);
  }
  return ret;
}
}  // namespace details
template <typename T, typename C>
void Maximum(NDArray<T, C> const &array1, NDArray<T, C> const &array2, NDArray<T, C> &ret)
{
  assert(ret.shape() == array1.shape());
  assert(array1.shape() == array2.shape());
  details::MaximumImplementation(array1, array2, ret);
}
template <typename T, typename C>
NDArray<T, C> &Maximum(NDArray<T, C> const &array1, NDArray<T, C> const &array2)
{
  NDArray<T, C> ret;
  Maximum(array1, array2, ret);
  return ret;
}
template <typename T, typename C>
void Maximum(ShapeLessArray<T, C> const &array1, ShapeLessArray<T, C> const &array2,
             ShapeLessArray<T, C> &ret)
{
  details::MaximumImplementation(array1, array2, ret);
}
template <typename T, typename C>
ShapeLessArray<T, C> Maximum(ShapeLessArray<T, C> const &array1, ShapeLessArray<T, C> const &array2)
{
  ShapeLessArray<T, C> ret;
  Maximum(array1, array2, ret);
  return ret;
}
/**
 * add a scalar to every value in the array
 * @tparam T
 * @tparam C
 * @param array1
 * @param scalar
 * @param ret
 */
template <typename T, typename C>
void Add(ShapeLessArray<T, C> const &array, T const &scalar, ShapeLessArray<T, C> &ret)
{
  assert(array.size() == ret.size());
  typename ShapeLessArray<T, C>::vector_register_type val(scalar);

  ret.data().in_parallel().Apply(
      [val](typename ShapeLessArray<T, C>::vector_register_type const &x,
            typename ShapeLessArray<T, C>::vector_register_type &      z) { z = x + val; },
      array.data());
}
template <typename T, typename C>
ShapeLessArray<T, C> Add(ShapeLessArray<T, C> const &array, T const &scalar)
{
  ShapeLessArray<T, C> ret;
  Add(array, scalar, ret);
  return ret;
}
template <typename T, typename C>
void Add(T const &scalar, ShapeLessArray<T, C> const &array, ShapeLessArray<T, C> &ret)
{
  ret = Add(array, scalar, ret);
}
template <typename T, typename C>
ShapeLessArray<T, C> Add(T const &scalar, ShapeLessArray<T, C> const &array)
{
  ShapeLessArray<T, C> ret;
  Add(scalar, array, ret);
  return ret;
}
/**
 * Adds two arrays together
 * @tparam T
 * @tparam C
 * @param array1
 * @param array2
 * @param ret
 */
template <typename T, typename C>
void Add(ShapeLessArray<T, C> const &array1, ShapeLessArray<T, C> const &array2,
         ShapeLessArray<T, C> &ret)
{
  memory::Range range{0, std::min(array1.data().size(), array2.data().size()), 1};
  Add(array1, array2, range, ret);
}
template <typename T, typename C>
ShapeLessArray<T, C> Add(ShapeLessArray<T, C> const &array1, ShapeLessArray<T, C> const &array2)
{
  ShapeLessArray<T, C> ret;
  Add(array1, array2, ret);
  return ret;
}
template <typename T, typename C>
void Add(ShapeLessArray<T, C> const &array1, ShapeLessArray<T, C> const &array2,
         memory::Range const &range, ShapeLessArray<T, C> &ret)
{
  assert(array1.size() == array2.size());
  ret.LazyResize(array1.size());

  if (range.is_undefined())
  {
    Add(array1, array2, ret);
  }
  else if (range.is_trivial())
  {
    auto r = range.ToTrivialRange(ret.data().size());

    ret.data().in_parallel().Apply(
        r,
        [](typename ShapeLessArray<T, C>::vector_register_type const &x,
           typename ShapeLessArray<T, C>::vector_register_type const &y,
           typename ShapeLessArray<T, C>::vector_register_type &      z) { z = x + y; },
        array1.data(), array2.data());
  }
  else
  {
    TODO_FAIL("Non-trivial ranges not implemented");
  }
}
template <typename T, typename C>
ShapeLessArray<T, C> Add(ShapeLessArray<T, C> const &array1, ShapeLessArray<T, C> const &array2,
                         memory::Range const &range)
{
  ShapeLessArray<T, C> ret;
  Add(array1, array2, range, ret);
  return ret;
}
/**
 * Adds two ndarrays together with broadcasting
 * @tparam T
 * @tparam C
 * @param array1
 * @param array2
 * @param range
 * @param ret
 */
template <typename T, typename C>
void Add(NDArray<T, C> &array1, NDArray<T, C> &array2, NDArray<T, C> &ret)
{
  Broadcast([](T x, T y) { return x + y; }, array1, array2, ret);
}
template <typename T, typename C>
NDArray<T, C> Add(NDArray<T, C> &array1, NDArray<T, C> &array2)
{
  NDArray<T, C> ret;
  Add(array1, array2, ret);
  return ret;
}
/**
 * subtract a scalar from every value in the array
 * @tparam T
 * @tparam C
 * @param array1
 * @param scalar
 * @param ret
 */
template <typename T, typename C>
void Subtract(ShapeLessArray<T, C> const &array, T const &scalar, ShapeLessArray<T, C> &ret)
{
  assert(array.size() == ret.size());
  assert(array.data().size() == ret.data().size());

  typename ShapeLessArray<T, C>::vector_register_type val(scalar);

  ret.data().in_parallel().Apply(
      [val](typename ShapeLessArray<T, C>::vector_register_type const &x,
            typename ShapeLessArray<T, C>::vector_register_type &      z) { z = x - val; },
      array.data());
}
template <typename T, typename C>
ShapeLessArray<T, C> Subtract(ShapeLessArray<T, C> const &array, T const &scalar)
{
  ShapeLessArray<T, C> ret;
  Subtract(array, scalar, ret);
  return ret;
}
/**
 * subtract a every value in array from scalar
 * @tparam T
 * @tparam C
 * @param array1
 * @param scalar
 * @param ret
 */
template <typename T, typename C>
void Subtract(T const &scalar, ShapeLessArray<T, C> const &array, ShapeLessArray<T, C> &ret)
{
  assert(array.size() == ret.size());
  for (std::size_t i = 0; i < ret.size(); ++i)
  {
    ret[i] = scalar - array[i];
  }
}
template <typename T, typename C>
ShapeLessArray<T, C> Subtract(T const &scalar, ShapeLessArray<T, C> const &array)
{
  ShapeLessArray<T, C> ret;
  Subtract(scalar, array, ret);
  return ret;
}
/**
 * subtract array from another array within a range
 * @tparam T
 * @tparam C
 * @param array1
 * @param scalar
 * @param ret
 */
template <typename T, typename C>
void Subtract(ShapeLessArray<T, C> const &obj1, ShapeLessArray<T, C> const &obj2,
              memory::Range const &range, ShapeLessArray<T, C> &ret)
{
  assert(obj1.size() == obj2.size());
  assert(obj1.size() == ret.size());

  if (range.is_undefined())
  {
    Subtract(obj1, obj2, ret);
  }
  else if (range.is_trivial())
  {
    auto r = range.ToTrivialRange(ret.data().size());

    ret.data().in_parallel().Apply(
        r,
        [](typename ShapeLessArray<T, C>::vector_register_type const &x,
           typename ShapeLessArray<T, C>::vector_register_type const &y,
           typename ShapeLessArray<T, C>::vector_register_type &      z) { z = x - y; },
        obj1.data(), obj2.data());
  }
  else
  {
    TODO_FAIL("Non-trivial ranges not implemented");
  }
}
template <typename T, typename C>
ShapeLessArray<T, C> Subtract(ShapeLessArray<T, C> const &obj1, ShapeLessArray<T, C> const &obj2,
                              memory::Range const &range)
{
  ShapeLessArray<T, C> ret;
  Subtract(obj1, obj2, range, ret);
  return ret;
}
/**
 * subtract array from another array
 * @tparam T
 * @tparam C
 * @param array1
 * @param scalar
 * @param ret
 */
template <typename T, typename C>
void Subtract(ShapeLessArray<T, C> const &obj1, ShapeLessArray<T, C> const &obj2,
              ShapeLessArray<T, C> &ret)
{
  memory::Range range{0, std::min(obj1.data().size(), obj1.data().size()), 1};
  Subtract(obj1, obj2, range, ret);
}
template <typename T, typename C>
ShapeLessArray<T, C> Subtract(ShapeLessArray<T, C> const &obj1, ShapeLessArray<T, C> const &obj2)
{
  ShapeLessArray<T, C> ret;
  Subtract(obj1, obj2, ret);
  return ret;
}
/**
 * subtract array from another array with broadcasting
 * @tparam T
 * @tparam C
 * @param array1
 * @param scalar
 * @param ret
 */
template <typename T, typename C>
void Subtract(NDArray<T, C> &obj1, NDArray<T, C> &obj2, NDArray<T, C> &ret)
{
  Broadcast([](T x, T y) { return x - y; }, obj1, obj2, ret);
}
template <typename T, typename C>
NDArray<T, C> Subtract(NDArray<T, C> &obj1, NDArray<T, C> &obj2)
{
  NDArray<T, C> ret;
  Subtract(obj1, obj2, ret);
  return ret;
}
/**
 * multiply a scalar by every value in the array
 * @tparam T
 * @tparam C
 * @param array1
 * @param scalar
 * @param ret
 */
template <typename T, typename C>
void Multiply(ShapeLessArray<T, C> const &array, T const &scalar, ShapeLessArray<T, C> &ret)
{
  assert(array.size() == ret.size());
  typename ShapeLessArray<T, C>::vector_register_type val(scalar);

  ret.data().in_parallel().Apply(
      [val](typename ShapeLessArray<T, C>::vector_register_type const &x,
            typename ShapeLessArray<T, C>::vector_register_type &      z) { z = x * val; },
      array.data());
}
template <typename T, typename C>
ShapeLessArray<T, C> Multiply(ShapeLessArray<T, C> const &array, T const &scalar)
{
  ShapeLessArray<T, C> ret;
  Multiply(array, scalar, ret);
  return ret;
}
template <typename T, typename C>
void Multiply(T const &scalar, ShapeLessArray<T, C> const &array, ShapeLessArray<T, C> &ret)
{
  Multiply(array, scalar, ret);
}
template <typename T, typename C>
ShapeLessArray<T, C> Multiply(T const &scalar, ShapeLessArray<T, C> const &array)
{
  ShapeLessArray<T, C> ret;
  Multiply(scalar, array, ret);
  return ret;
}

/**
 * Multiply array by another array within a range
 * @tparam T
 * @tparam C
 * @param array1
 * @param scalar
 * @param ret
 */
template <typename T, typename C>
void Multiply(ShapeLessArray<T, C> const &obj1, ShapeLessArray<T, C> const &obj2,
              memory::Range const &range, ShapeLessArray<T, C> &ret)
{
  assert(obj1.size() == obj2.size());
  assert(obj1.size() == ret.size());

  if (range.is_undefined())
  {
    Multiply(obj1, obj2, ret);
  }
  else if (range.is_trivial())
  {
    auto r = range.ToTrivialRange(ret.data().size());

    ret.data().in_parallel().Apply(
        r,
        [](typename ShapeLessArray<T, C>::vector_register_type const &x,
           typename ShapeLessArray<T, C>::vector_register_type const &y,
           typename ShapeLessArray<T, C>::vector_register_type &      z) { z = x * y; },
        obj1.data(), obj2.data());
  }
  else
  {
    TODO_FAIL("Non-trivial ranges not implemented");
  }
}
template <typename T, typename C>
ShapeLessArray<T, C> Multiply(ShapeLessArray<T, C> const &obj1, ShapeLessArray<T, C> const &obj2,
                              memory::Range const &range)
{
  ShapeLessArray<T, C> ret;
  Multiply(obj1, obj2, range, ret);
  return ret;
}
/**
 * Multiply array from another array
 * @tparam T
 * @tparam C
 * @param array1
 * @param scalar
 * @param ret
 */
template <typename T, typename C>
void Multiply(ShapeLessArray<T, C> const &obj1, ShapeLessArray<T, C> const &obj2,
              ShapeLessArray<T, C> &ret)
{
  memory::Range range{0, std::min(obj1.data().size(), obj2.data().size()), 1};
  Multiply(obj1, obj2, range, ret);
}
template <typename T, typename C>
ShapeLessArray<T, C> Multiply(ShapeLessArray<T, C> const &obj1, ShapeLessArray<T, C> const &obj2)
{
  ShapeLessArray<T, C> ret;
  Multiply(obj1, obj2, ret);
  return ret;
}
/**
 * Multiply array by another array with broadcasting
 * @tparam T
 * @tparam C
 * @param array1
 * @param scalar
 * @param ret
 */
template <typename T, typename C>
void Multiply(NDArray<T, C> &obj1, NDArray<T, C> &obj2, NDArray<T, C> &ret)
{
  Broadcast([](T x, T y) { return x * y; }, obj1, obj2, ret);
}
template <typename T, typename C>
NDArray<T, C> Multiply(NDArray<T, C> &obj1, NDArray<T, C> &obj2)
{
  NDArray<T, C> ret;
  Multiply(obj1, obj2, ret);
  return ret;
}
/**
 * divide array by a scalar
 * @tparam T
 * @tparam C
 * @param array1
 * @param scalar
 * @param ret
 */
template <typename T, typename C>
void Divide(ShapeLessArray<T, C> const &array, T const &scalar, ShapeLessArray<T, C> &ret)
{
  assert(array.size() == ret.size());
  typename ShapeLessArray<T, C>::vector_register_type val(scalar);

  ret.data().in_parallel().Apply(
      [val](typename ShapeLessArray<T, C>::vector_register_type const &x,
            typename ShapeLessArray<T, C>::vector_register_type &      z) { z = x / val; },
      array.data());
}
template <typename T, typename C>
ShapeLessArray<T, C> Divide(ShapeLessArray<T, C> const &array, T const &scalar)
{
  ShapeLessArray<T, C> ret;
  Divide(array, scalar, ret);
  return ret;
}
/**
 * elementwise divide scalar by array element
 * @tparam T
 * @tparam C
 * @param scalar
 * @param array
 * @param ret
 */
template <typename T, typename C>
void Divide(T const &scalar, ShapeLessArray<T, C> const &array, ShapeLessArray<T, C> &ret)
{
  assert(array.size() == ret.size());
  typename ShapeLessArray<T, C>::vector_register_type val(scalar);

  ret.data().in_parallel().Apply(
      [val](typename ShapeLessArray<T, C>::vector_register_type const &x,
            typename ShapeLessArray<T, C>::vector_register_type &      z) { z = val / x; },
      array.data());
}
template <typename T, typename C>
ShapeLessArray<T, C> Divide(T const &scalar, ShapeLessArray<T, C> const &array)
{
  ShapeLessArray<T, C> ret;
  Divide(scalar, array, ret);
  return ret;
}
/**
 * Divide array by another array within a range
 * @tparam T
 * @tparam C
 * @param array1
 * @param scalar
 * @param ret
 */
template <typename T, typename C>
void Divide(ShapeLessArray<T, C> const &obj1, ShapeLessArray<T, C> const &obj2,
            memory::Range const &range, ShapeLessArray<T, C> &ret)
{
  assert(obj1.size() == obj2.size());
  assert(obj1.size() == ret.size());

  if (range.is_undefined())
  {
    Divide(obj1, obj2, ret);
  }
  else if (range.is_trivial())
  {
    auto r = range.ToTrivialRange(ret.data().size());

    ret.data().in_parallel().Apply(
        r,
        [](typename ShapeLessArray<T, C>::vector_register_type const &x,
           typename ShapeLessArray<T, C>::vector_register_type const &y,
           typename ShapeLessArray<T, C>::vector_register_type &      z) { z = x / y; },
        obj1.data(), obj2.data());
  }
  else
  {
    TODO_FAIL("Non-trivial ranges not implemented");
  }
}
template <typename T, typename C>
void Divide(ShapeLessArray<T, C> const &obj1, ShapeLessArray<T, C> const &obj2,
            memory::Range const &range)
{
  ShapeLessArray<T, C> ret;
  Divide(obj1, obj2, range, ret);
  return ret;
}
/**
 * subtract array from another array
 * @tparam T
 * @tparam C
 * @param array1
 * @param scalar
 * @param ret
 */
template <typename T, typename C>
void Divide(ShapeLessArray<T, C> const &obj1, ShapeLessArray<T, C> const &obj2,
            ShapeLessArray<T, C> &ret)
{
  memory::Range range{0, std::min(obj1.data().size(), obj1.data().size()), 1};
  Divide(obj1, obj2, range, ret);
}
template <typename T, typename C>
ShapeLessArray<T, C> Divide(ShapeLessArray<T, C> const &obj1, ShapeLessArray<T, C> const &obj2)
{
  ShapeLessArray<T, C> ret;
  Divide(obj1, obj2, ret);
  return ret;
}
/**
 * subtract array from another array with broadcasting
 * @tparam T
 * @tparam C
 * @param array1
 * @param scalar
 * @param ret
 */
template <typename T, typename C>
void Divide(NDArray<T, C> &obj1, NDArray<T, C> &obj2, NDArray<T, C> &ret)
{
  Broadcast([](T x, T y) { return x / y; }, obj1, obj2, ret);
}
template <typename T, typename C>
NDArray<T, C> Divide(NDArray<T, C> &obj1, NDArray<T, C> &obj2)
{
  NDArray<T, C> ret;
  Divide(obj1, obj2, ret);
  return ret;
}

/**
 * return the product of all elements in the array
 * @tparam T
 * @tparam C
 * @param obj1
 * @param ret
 */
template <typename T, typename C>
void Product(ShapeLessArray<T, C> const &obj1, T &ret)
{
  ret = obj1.data().in_parallel().Reduce(
      memory::TrivialRange(0, obj1.size()),
      [](typename ShapeLessArray<T, C>::vector_register_type const &a,
         typename ShapeLessArray<T, C>::vector_register_type const &b) ->
      typename ShapeLessArray<T, C>::vector_register_type { return a * b; });
}
template <typename T, typename C>
void Product(ShapeLessArray<T, C> const &obj1)
{
  T ret;
  Product(obj1, ret);
  return ret;
}

/**
 * return the product of all elements in the array
 * @tparam T
 * @tparam C
 * @param obj1
 * @param ret
 */
template <typename T, typename C>
void Sum(ShapeLessArray<T, C> const &obj1, T &ret)
{
  ret = obj1.data().in_parallel().Reduce(
      memory::TrivialRange(0, obj1.size()),
      [](typename ShapeLessArray<T, C>::vector_register_type const &a,
         typename ShapeLessArray<T, C>::vector_register_type const &b) ->
      typename ShapeLessArray<T, C>::vector_register_type { return a + b; });
}
template <typename T, typename C>
T Sum(ShapeLessArray<T, C> const &obj1)
{
  T ret;
  Sum(obj1, ret);
  return ret;
}

}  // namespace math
}  // namespace fetch
