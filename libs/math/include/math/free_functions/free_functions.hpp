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
struct Scatter
{
  void operator()(NDArray<T, C> &input_array, std::vector<T> &updates,
                  std::vector<std::uint64_t> &indices) const
  {
    // sort indices and updates into ascending order

    std::vector<std::pair<std::uint64_t, T>> AB;

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
    NDArrayIterator<T, typename NDArray<T, C>::container_type> arr_iterator{input_array};

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
};

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
template <typename T, typename C>
void Abs(NDArray<T, C> &x)
{
  kernels::stdlib::Abs<T> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}
template <typename T, typename C>
void Abs(ShapeLessArray<T, C> &x)
{
  kernels::stdlib::Abs<T> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 * e^x
 * @param x
 */
template <typename T, typename C>
void Exp(NDArray<T, C> &x)
{
  kernels::stdlib::Exp<T> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}
template <typename T, typename C>
void Exp(ShapeLessArray<T, C> &x)
{
  kernels::stdlib::Exp<T> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 * raise 2 to power input values of x
 * @param x
 */
template <typename T, typename C>
void Exp2(NDArray<T, C> &x)
{
  kernels::stdlib::Exp<T> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}
template <typename T, typename C>
void Exp2(ShapeLessArray<T, C> &x)
{
  kernels::stdlib::Exp2<T> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 * exp(x) - 1
 * @param x
 */
template <typename T, typename C>
void Expm1(NDArray<T, C> &x)
{
  kernels::stdlib::Expm1<T> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}
template <typename T, typename C>
void Expm1(ShapeLessArray<T, C> &x)
{
  kernels::stdlib::Expm1<T> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 * natural logarithm of x
 * @param x
 */
template <typename T, typename C>
void Log(NDArray<T, C> &x)
{
  kernels::stdlib::Log<T> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}
template <typename T, typename C>
void Log(ShapeLessArray<T, C> &x)
{
  kernels::stdlib::Log<T> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 * natural logarithm of x
 * @param x
 */
template <typename T, typename C>
void Log10(NDArray<T, C> &x)
{
  kernels::stdlib::Log10<T> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}
template <typename T, typename C>
void Log10(ShapeLessArray<T, C> &x)
{
  kernels::stdlib::Log10<T> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 * log base 2
 * @param x
 */
template <typename T, typename C>
void Log2(NDArray<T, C> &x)
{
  kernels::stdlib::Log2<T> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}
template <typename T, typename C>
void Log2(ShapeLessArray<T, C> &x)
{
  kernels::stdlib::Log2<T> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 * natural log 1 + x
 * @param x
 */
template <typename T, typename C>
void Log1p(NDArray<T, C> &x)
{
  kernels::stdlib::Log1p<T> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}
template <typename T, typename C>
void Log1p(ShapeLessArray<T, C> &x)
{
  kernels::stdlib::Log1p<T> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 * square root
 * @param x
 */
template <typename T, typename C>
void Sqrt(NDArray<T, C> &x)
{
  kernels::stdlib::Sqrt<T> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}
template <typename T, typename C>
void Sqrt(ShapeLessArray<T, C> &x)
{
  kernels::stdlib::Sqrt<T> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 * cubic root x
 * @param x
 */
template <typename T, typename C>
void Cbrt(NDArray<T, C> &x)
{
  kernels::stdlib::Cbrt<T> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}
template <typename T, typename C>
void Cbrt(ShapeLessArray<T, C> &x)
{
  kernels::stdlib::Cbrt<T> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 * raise to power
 * @param x
 */
template <typename T, typename C>
void Pow(NDArray<T, C> &x)
{
  kernels::stdlib::Pow<T> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}
template <typename T, typename C>
void Pow(ShapeLessArray<T, C> &x)
{
  kernels::stdlib::Pow<T> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 * sine of x
 * @param x
 */
template <typename T, typename C>
void Sin(NDArray<T, C> &x)
{
  kernels::stdlib::Sin<T> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}
template <typename T, typename C>
void Sin(ShapeLessArray<T, C> &x)
{
  kernels::stdlib::Sin<T> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 * cosine of x
 * @param x
 */
template <typename T, typename C>
void Cos(NDArray<T, C> &x)
{
  kernels::stdlib::Cos<T> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}
template <typename T, typename C>
void Cos(ShapeLessArray<T, C> &x)
{
  kernels::stdlib::Cos<T> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 * tangent of x
 * @param x
 */
template <typename T, typename C>
void Tan(NDArray<T, C> &x)
{
  kernels::stdlib::Tan<T> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}
template <typename T, typename C>
void Tan(ShapeLessArray<T, C> &x)
{
  kernels::stdlib::Tan<T> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 * arc sine of x
 * @param x
 */
template <typename T, typename C>
void Asin(NDArray<T, C> &x)
{
  kernels::stdlib::Asin<T> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}
template <typename T, typename C>
void Asin(ShapeLessArray<T, C> &x)
{
  kernels::stdlib::Asin<T> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 * arc cosine of x
 * @param x
 */
template <typename T, typename C>
void Acos(NDArray<T, C> &x)
{
  kernels::stdlib::Acos<T> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}
template <typename T, typename C>
void Acos(ShapeLessArray<T, C> &x)
{
  kernels::stdlib::Acos<T> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 * arc tangent of x
 * @param x
 */
template <typename T, typename C>
void Atan(NDArray<T, C> &x)
{
  kernels::stdlib::Atan<T> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}
template <typename T, typename C>
void Atan(ShapeLessArray<T, C> &x)
{
  kernels::stdlib::Atan<T> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 * arc tangent of x
 * @param x
 */
template <typename T, typename C>
void Atan2(NDArray<T, C> &x)
{
  kernels::stdlib::Atan2<T> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}
template <typename T, typename C>
void Atan2(ShapeLessArray<T, C> &x)
{
  kernels::stdlib::Atan2<T> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 * hyperbolic sine of x
 * @param x
 */
template <typename T, typename C>
void Sinh(NDArray<T, C> &x)
{
  kernels::stdlib::Sinh<T> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}
template <typename T, typename C>
void Sinh(ShapeLessArray<T, C> &x)
{
  kernels::stdlib::Sinh<T> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 * hyperbolic cosine of x
 * @param x
 */
template <typename T, typename C>
void Cosh(NDArray<T, C> &x)
{
  kernels::stdlib::Cosh<T> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}
template <typename T, typename C>
void Cosh(ShapeLessArray<T, C> &x)
{
  kernels::stdlib::Cosh<T> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 * hyperbolic tangent of x
 * @param x
 */
template <typename T, typename C>
void Tanh(NDArray<T, C> &x)
{
  kernels::stdlib::Tanh<T> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}
template <typename T, typename C>
void Tanh(ShapeLessArray<T, C> &x)
{
  kernels::stdlib::Tanh<T> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 * hyperbolic arc sine of x
 * @param x
 */
template <typename T, typename C>
void Asinh(NDArray<T, C> &x)
{
  kernels::stdlib::Asinh<T> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}
template <typename T, typename C>
void Asinh(ShapeLessArray<T, C> &x)
{
  kernels::stdlib::Asinh<T> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 * hyperbolic arc cosine of x
 * @param x
 */
template <typename T, typename C>
void Acosh(NDArray<T, C> &x)
{
  kernels::stdlib::Acosh<T> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}
template <typename T, typename C>
void Acosh(ShapeLessArray<T, C> &x)
{
  kernels::stdlib::Acosh<T> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 * hyperbolic arc tangent of x
 * @param x
 */
template <typename T, typename C>
void Atanh(NDArray<T, C> &x)
{
  kernels::stdlib::Atanh<T> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}
template <typename T, typename C>
void Atanh(ShapeLessArray<T, C> &x)
{
  kernels::stdlib::Atanh<T> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 * error function of x
 * @param x
 */
template <typename T, typename C>
void Erf(NDArray<T, C> &x)
{
  kernels::stdlib::Erf<T> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}
template <typename T, typename C>
void Erf(ShapeLessArray<T, C> &x)
{
  kernels::stdlib::Erf<T> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 * complementary error function of x
 * @param x
 */
template <typename T, typename C>
void Erfc(NDArray<T, C> &x)
{
  kernels::stdlib::Erfc<T> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}
template <typename T, typename C>
void Erfc(ShapeLessArray<T, C> &x)
{
  kernels::stdlib::Erfc<T> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 * factorial of x-1
 * @param x
 */
template <typename T, typename C>
void Tgamma(NDArray<T, C> &x)
{
  kernels::stdlib::Tgamma<T> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}
template <typename T, typename C>
void Tgamma(ShapeLessArray<T, C> &x)
{
  kernels::stdlib::Tgamma<T> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 * log of factorial of x-1
 * @param x
 */
template <typename T, typename C>
void Lgamma(NDArray<T, C> &x)
{
  kernels::stdlib::Lgamma<T> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}
template <typename T, typename C>
void Lgamma(ShapeLessArray<T, C> &x)
{
  kernels::stdlib::Lgamma<T> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 * ceiling round
 * @param x
 */
template <typename T, typename C>
void Ceil(NDArray<T, C> &x)
{
  kernels::stdlib::Ceil<T> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}
template <typename T, typename C>
void Ceil(ShapeLessArray<T, C> &x)
{
  kernels::stdlib::Ceil<T> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 * floor rounding
 * @param x
 */
template <typename T, typename C>
void Floor(NDArray<T, C> &x)
{
  kernels::stdlib::Floor<T> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}
template <typename T, typename C>
void Floor(ShapeLessArray<T, C> &x)
{
  kernels::stdlib::Floor<T> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 * round towards 0
 * @param x
 */
template <typename T, typename C>
void Trunc(NDArray<T, C> &x)
{
  kernels::stdlib::Trunc<T> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}
template <typename T, typename C>
void Trunc(ShapeLessArray<T, C> &x)
{
  kernels::stdlib::Trunc<T> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 * round to nearest int in int format
 * @param x
 */
template <typename T, typename C>
void Round(NDArray<T, C> &x)
{
  kernels::stdlib::Round<T> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}
template <typename T, typename C>
void Round(ShapeLessArray<T, C> &x)
{
  kernels::stdlib::Round<T> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 * round to nearest int in float format
 * @param x
 */
template <typename T, typename C>
void Lround(NDArray<T, C> &x)
{
  kernels::stdlib::Lround<T> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}
template <typename T, typename C>
void Lround(ShapeLessArray<T, C> &x)
{
  kernels::stdlib::Lround<T> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 * round to nearest int in float format with long long return
 * @param x
 */
template <typename T, typename C>
void Llround(NDArray<T, C> &x)
{
  kernels::stdlib::Llround<T> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}
template <typename T, typename C>
void Llround(ShapeLessArray<T, C> &x)
{
  kernels::stdlib::Llround<T> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 * round to nearest int in float format
 * @param x
 */
template <typename T, typename C>
void Nearbyint(NDArray<T, C> &x)
{
  kernels::stdlib::Nearbyint<T> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}
template <typename T, typename C>
void Nearbyint(ShapeLessArray<T, C> &x)
{
  kernels::stdlib::Nearbyint<T> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 * round to nearest int
 * @param x
 */
template <typename T, typename C>
void Rint(NDArray<T, C> &x)
{
  kernels::stdlib::Rint<T> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}
template <typename T, typename C>
void Rint(ShapeLessArray<T, C> &x)
{
  kernels::stdlib::Rint<T> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 *
 * @param x
 */
template <typename T, typename C>
void Lrint(NDArray<T, C> &x)
{
  kernels::stdlib::Lrint<T> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}
template <typename T, typename C>
void Lrint(ShapeLessArray<T, C> &x)
{
  kernels::stdlib::Lrint<T> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 *
 * @param x
 */
template <typename T, typename C>
void Llrint(NDArray<T, C> &x)
{
  kernels::stdlib::Llrint<T> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}
template <typename T, typename C>
void Llrint(ShapeLessArray<T, C> &x)
{
  kernels::stdlib::Llrint<T> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 * finite check
 * @param x
 */
template <typename T, typename C>
void Isfinite(NDArray<T, C> &x)
{
  kernels::stdlib::Isfinite<T> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}
template <typename T, typename C>
void Isfinite(ShapeLessArray<T, C> &x)
{
  kernels::stdlib::Isfinite<T> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 * checks for inf values
 * @param x
 */
template <typename T, typename C>
void Isinf(NDArray<T, C> &x)
{
  kernels::stdlib::Isinf<T> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}
template <typename T, typename C>
void Isinf(ShapeLessArray<T, C> &x)
{
  kernels::stdlib::Isinf<T> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 * checks for nans
 * @param x
 */
template <typename T, typename C>
void Isnan(NDArray<T, C> &x)
{
  kernels::stdlib::Isnan<T> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}
template <typename T, typename C>
void Isnan(ShapeLessArray<T, C> &x)
{
  kernels::stdlib::Isnan<T> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 * rectified linear activation function
 * @param x
 */
template <typename T, typename C>
void Relu(NDArray<T, C> &x)
{
  kernels::Relu<typename NDArray<T, C>::vector_register_type> relu;
  x.data().in_parallel().Apply(relu, x.data());
}
template <typename T, typename C>
void Relu(ShapeLessArray<T, C> &x)
{
  kernels::Relu<typename ShapeLessArray<T, C>::vector_register_type> relu;
  x.data().in_parallel().Apply(relu, x.data());
}

/**
 * replaces data with the sign (1 or -1)
 * @param x
 */
template <typename T, typename C>
void Sign(NDArray<T, C> &x)
{
  kernels::Sign<typename NDArray<T, C>::vector_register_type> sign;
  x.data().in_parallel().Apply(sign, x.data());
}
template <typename T, typename C>
void Sign(ShapeLessArray<T, C> &x)
{
  kernels::Sign<typename ShapeLessArray<T, C>::vector_register_type> sign;
  x.data().in_parallel().Apply(sign, x.data());
}

/**
 *
 * @param x
 */
template <typename T, typename C>
void Hypot(NDArray<T, C> &x)
{
  kernels::stdlib::Hypot<T> sign;
  x.data().in_parallel().Apply(sign, x.data());
}
template <typename T, typename C>
void Hypot(ShapeLessArray<T, C> &x)
{
  kernels::stdlib::Hypot<T> sign;
  x.data().in_parallel().Apply(sign, x.data());
}

/**
 *
 * @param x
 */
template <typename T, typename C>
void Frexp(NDArray<T, C> &x)
{
  kernels::stdlib::Frexp<T> sign;
  x.data().in_parallel().Apply(sign, x.data());
}
template <typename T, typename C>
void Frexp(ShapeLessArray<T, C> &x)
{
  kernels::stdlib::Frexp<T> sign;
  x.data().in_parallel().Apply(sign, x.data());
}

/**
 *
 * @param x
 */
template <typename T, typename C>
void Ldexp(NDArray<T, C> &x)
{
  kernels::stdlib::Ldexp<T> sign;
  x.data().in_parallel().Apply(sign, x.data());
}
template <typename T, typename C>
void Ldexp(ShapeLessArray<T, C> &x)
{
  kernels::stdlib::Ldexp<T> sign;
  x.data().in_parallel().Apply(sign, x.data());
}

/**
 *
 * @param x
 */
template <typename T, typename C>
void Modf(NDArray<T, C> &x)
{
  kernels::stdlib::Modf<T> sign;
  x.data().in_parallel().Apply(sign, x.data());
}
template <typename T, typename C>
void Modf(ShapeLessArray<T, C> &x)
{
  kernels::stdlib::Modf<T> sign;
  x.data().in_parallel().Apply(sign, x.data());
}

/**
 *
 * @param x
 */
template <typename T, typename C>
void Scalbn(NDArray<T, C> &x)
{
  kernels::stdlib::Scalbn<T> sign;
  x.data().in_parallel().Apply(sign, x.data());
}
template <typename T, typename C>
void Scalbn(ShapeLessArray<T, C> &x)
{
  kernels::stdlib::Scalbn<T> sign;
  x.data().in_parallel().Apply(sign, x.data());
}

/**
 *
 * @param x
 */
template <typename T, typename C>
void Scalbln(NDArray<T, C> &x)
{
  kernels::stdlib::Scalbln<T> sign;
  x.data().in_parallel().Apply(sign, x.data());
}
template <typename T, typename C>
void Scalbln(ShapeLessArray<T, C> &x)
{
  kernels::stdlib::Scalbln<T> sign;
  x.data().in_parallel().Apply(sign, x.data());
}

/**
 *
 * @param x
 */
template <typename T, typename C>
void Ilogb(NDArray<T, C> &x)
{
  kernels::stdlib::Ilogb<T> sign;
  x.data().in_parallel().Apply(sign, x.data());
}
template <typename T, typename C>
void Ilogb(ShapeLessArray<T, C> &x)
{
  kernels::stdlib::Ilogb<T> sign;
  x.data().in_parallel().Apply(sign, x.data());
}

/**
 *
 * @param x
 */
template <typename T, typename C>
void Logb(NDArray<T, C> &x)
{
  kernels::stdlib::Logb<T> sign;
  x.data().in_parallel().Apply(sign, x.data());
}
template <typename T, typename C>
void Logb(ShapeLessArray<T, C> &x)
{
  kernels::stdlib::Logb<T> sign;
  x.data().in_parallel().Apply(sign, x.data());
}

/**
 *
 * @param x
 */
template <typename T, typename C>
void Nextafter(NDArray<T, C> &x)
{
  kernels::stdlib::Nextafter<T> sign;
  x.data().in_parallel().Apply(sign, x.data());
}
template <typename T, typename C>
void Nextafter(ShapeLessArray<T, C> &x)
{
  kernels::stdlib::Nextafter<T> sign;
  x.data().in_parallel().Apply(sign, x.data());
}

/**
 *
 * @param x
 */
template <typename T, typename C>
void Nexttoward(NDArray<T, C> &x)
{
  kernels::stdlib::Nexttoward<T> sign;
  x.data().in_parallel().Apply(sign, x.data());
}
template <typename T, typename C>
void Nexttoward(ShapeLessArray<T, C> &x)
{
  kernels::stdlib::Nexttoward<T> sign;
  x.data().in_parallel().Apply(sign, x.data());
}

/**
 *
 * @param x
 */
template <typename T, typename C>
void Copysign(NDArray<T, C> &x)
{
  kernels::stdlib::Copysign<T> sign;
  x.data().in_parallel().Apply(sign, x.data());
}
template <typename T, typename C>
void Copysign(ShapeLessArray<T, C> &x)
{
  kernels::stdlib::Copysign<T> sign;
  x.data().in_parallel().Apply(sign, x.data());
}

/**
 *
 * @param x
 */
template <typename T, typename C>
void Fpclassify(NDArray<T, C> &x)
{
  kernels::stdlib::Fpclassify<T> sign;
  x.data().in_parallel().Apply(sign, x.data());
}
template <typename T, typename C>
void Fpclassify(ShapeLessArray<T, C> &x)
{
  kernels::stdlib::Fpclassify<T> sign;
  x.data().in_parallel().Apply(sign, x.data());
}

/**
 *
 * @param x
 */
template <typename T, typename C>
void Isnormal(NDArray<T, C> &x)
{
  kernels::stdlib::Isnormal<T> sign;
  x.data().in_parallel().Apply(sign, x.data());
}
template <typename T, typename C>
void Isnormal(ShapeLessArray<T, C> &x)
{
  kernels::stdlib::Isnormal<T> sign;
  x.data().in_parallel().Apply(sign, x.data());
}

/**
 *
 * @param x
 */
template <typename T, typename C>
void Signbit(NDArray<T, C> &x)
{
  kernels::stdlib::Signbit<T> sign;
  x.data().in_parallel().Apply(sign, x.data());
}
template <typename T, typename C>
void Signbit(ShapeLessArray<T, C> &x)
{
  kernels::stdlib::Signbit<T> sign;
  x.data().in_parallel().Apply(sign, x.data());
}

/**
 *
 * @param x
 */
template <typename T, typename C>
void Isgreater(NDArray<T, C> &x)
{
  kernels::stdlib::Isgreater<T> sign;
  x.data().in_parallel().Apply(sign, x.data());
}
template <typename T, typename C>
void Isgreater(ShapeLessArray<T, C> &x)
{
  kernels::stdlib::Isgreater<T> sign;
  x.data().in_parallel().Apply(sign, x.data());
}

/**
 *
 * @param x
 */
template <typename T, typename C>
void Isgreaterequal(NDArray<T, C> &x)
{
  kernels::stdlib::Isgreaterequal<T> sign;
  x.data().in_parallel().Apply(sign, x.data());
}
template <typename T, typename C>
void Isgreaterequal(ShapeLessArray<T, C> &x)
{
  kernels::stdlib::Isgreaterequal<T> sign;
  x.data().in_parallel().Apply(sign, x.data());
}

/**
 *
 * @param x
 */
template <typename T, typename C>
void Isless(NDArray<T, C> &x)
{
  kernels::stdlib::Isless<T> sign;
  x.data().in_parallel().Apply(sign, x.data());
}
template <typename T, typename C>
void Isless(ShapeLessArray<T, C> &x)
{
  kernels::stdlib::Isless<T> sign;
  x.data().in_parallel().Apply(sign, x.data());
}

/**
 *
 * @param x
 */
template <typename T, typename C>
void Islessequal(NDArray<T, C> &x)
{
  kernels::stdlib::Islessequal<T> sign;
  x.data().in_parallel().Apply(sign, x.data());
}
template <typename T, typename C>
void Islessequal(ShapeLessArray<T, C> &x)
{
  kernels::stdlib::Islessequal<T> sign;
  x.data().in_parallel().Apply(sign, x.data());
}

/**
 *
 * @param x
 */
template <typename T, typename C>
void Islessgreater(NDArray<T, C> &x)
{
  kernels::stdlib::Islessgreater<T> sign;
  x.data().in_parallel().Apply(sign, x.data());
}
template <typename T, typename C>
void Islessgreater(ShapeLessArray<T, C> &x)
{
  kernels::stdlib::Islessgreater<T> sign;
  x.data().in_parallel().Apply(sign, x.data());
}

/**
 *
 * @param x
 */
template <typename T, typename C>
void Isunordered(NDArray<T, C> &x)
{
  kernels::stdlib::Isunordered<T> sign;
  x.data().in_parallel().Apply(sign, x.data());
}
template <typename T, typename C>
void Isunordered(ShapeLessArray<T, C> &x)
{
  kernels::stdlib::Isunordered<T> sign;
  x.data().in_parallel().Apply(sign, x.data());
}

/**
 *
 * @param x
 */
template <typename T, typename C>
void ApproxExp(NDArray<T, C> &x)
{
  kernels::ApproxExp<typename NDArray<T, C>::vector_register_type> sign;
  x.data().in_parallel().Apply(sign, x.data());
}
template <typename T, typename C>
void ApproxExp(ShapeLessArray<T, C> &x)
{
  kernels::ApproxExp<typename ShapeLessArray<T, C>::vector_register_type> sign;
  x.data().in_parallel().Apply(sign, x.data());
}

/**
 *
 * @param x
 */
template <typename T, typename C>
void ApproxLog(NDArray<T, C> &x)
{
  kernels::ApproxLog<typename NDArray<T, C>::vector_register_type> sign;
  x.data().in_parallel().Apply(sign, x.data());
}
template <typename T, typename C>
void ApproxLog(ShapeLessArray<T, C> &x)
{
  kernels::ApproxLog<typename ShapeLessArray<T, C>::vector_register_type> sign;
  x.data().in_parallel().Apply(sign, x.data());
}

/**
 *
 * @param x
 */
template <typename T, typename C>
void ApproxLogistic(NDArray<T, C> &x)
{
  kernels::ApproxLogistic<typename NDArray<T, C>::vector_register_type> sign;
  x.data().in_parallel().Apply(sign, x.data());
}
template <typename T, typename C>
void ApproxLogistic(ShapeLessArray<T, C> &x)
{
  kernels::ApproxLogistic<typename ShapeLessArray<T, C>::vector_register_type> sign;
  x.data().in_parallel().Apply(sign, x.data());
}

}  // namespace math
}  // namespace fetch
