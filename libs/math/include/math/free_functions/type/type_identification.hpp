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

#include "math/kernels/standard_functions.hpp"
#include <cassert>
#include "math/meta/type_traits.hpp"

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

} // math
} // fetch