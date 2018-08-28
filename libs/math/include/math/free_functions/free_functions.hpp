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
#include "vectorise/memory/array.hpp"

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
// template <typename T, typename C>
template <typename ARRAY_TYPE>
void Scatter(ARRAY_TYPE &input_array, std::vector<typename ARRAY_TYPE::type> &updates,
             std::vector<std::uint64_t> &indices)
{
  // sort indices and updates into ascending order

  std::vector<std::pair<std::uint64_t, typename ARRAY_TYPE::type>> AB;

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

  assert(indices.back() <= input_array.shape()[0]);

  // set up an iterator
  NDArrayIterator<typename ARRAY_TYPE::type, typename ARRAY_TYPE::container_type> arr_iterator{
      input_array};

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
template <typename ARRAY_TYPE>
void Gather(ARRAY_TYPE &input_array, std::vector<std::uint64_t> &indices, ARRAY_TYPE &data)
{

  assert(input_array.size() == data.size());
  input_array.LazyReshape(data.shape());

  // sort indices
  std::sort(indices.begin(), indices.end());

  // check largest value in indices < shape()[0]
  assert(indices.back() <= data.shape()[0]);

  // set up an iterator
  NDArrayIterator<typename ARRAY_TYPE::type, typename ARRAY_TYPE::container_type> arr_iterator{
      data};
  NDArrayIterator<typename ARRAY_TYPE::type, typename ARRAY_TYPE::container_type> ret_iterator{
      input_array};

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
template <typename ARRAY_TYPE>
void DynamicStitch(ARRAY_TYPE &input_array, std::vector<std::vector<std::size_t>> const &indices,
                   std::vector<ARRAY_TYPE> const &data)
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

template <typename ARRAY_TYPE>
void BooleanMaskImplementation(ARRAY_TYPE &input_array, ARRAY_TYPE const &mask)
{
  assert(input_array.size() == mask.size());

  std::size_t counter = 0;
  for (std::size_t i = 0; i < input_array.size(); ++i)
  {
    assert((mask[i] == 1) || (mask[i] == 0));
    // TODO(private issue 193): implement boolean only ndarray to avoid cast
    if (bool(mask[i]))
    {
      input_array[counter] = input_array[i];
      ++counter;
    }
  }

  input_array.LazyResize(counter);
}
template <typename ARRAY_TYPE>
void BooleanMask(ARRAY_TYPE &input_array, ARRAY_TYPE const &mask)
{
  BooleanMaskImplementation(input_array, mask);
}
template <typename T, typename C>
void BooleanMask(NDArray<T, C> &input_array, NDArray<T, C> const &mask)
{
  assert(input_array.shape() >= mask.shape());

  BooleanMaskImplementation(input_array, mask);

  // figure out the output shape
  std::vector<std::size_t> new_shape;
  for (std::size_t i = 0; i < input_array.shape().size(); ++i)
  {
    if (!(mask.shape()[i] == input_array.shape()[i]))
    {
      new_shape.push_back(mask.shape()[i]);
    }
  }
  new_shape.push_back(input_array.size());

  input_array.ResizeFromShape(new_shape);
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
}  // namespace math
}  // namespace fetch
