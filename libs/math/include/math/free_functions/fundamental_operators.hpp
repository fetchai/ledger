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

#include "math/meta/type_traits.hpp"

/**
 * assigns the absolute of x to this array
 * @param x
 */

namespace fetch {
namespace math {


/**
 * add a scalar to every value in the array
 * @tparam T
 * @tparam C
 * @param array1
 * @param scalar
 * @param ret
 */
template <typename T, typename ArrayType>
meta::IsMathShapelessArrayLike<ArrayType, void> Add(ArrayType const &array, T const &scalar, ArrayType &ret)
{
  assert(array.size() == ret.size());
  typename ArrayType::vector_register_type val(scalar);

  ret.data().in_parallel().Apply(
      [val](typename ArrayType::vector_register_type const &x,
            typename ArrayType::vector_register_type &      z) { z = x + val; },
      array.data());
}
template <typename T, typename ArrayType>
meta::IsMathShapelessArrayLike<ArrayType, ArrayType> Add(ArrayType const &array, T const &scalar)
{
  ArrayType ret{array.size()};
  Add(array, scalar, ret);
  return ret;
}
template <typename T, typename ArrayType>
meta::IsMathShapelessArrayLike<ArrayType, void> Add(T const &scalar, ArrayType const &array, ArrayType &ret)
{
  ret = Add(array, scalar, ret);
}
template <typename T, typename ArrayType>
meta::IsMathShapelessArrayLike<ArrayType, ArrayType> Add(T const &scalar, ArrayType const &array)
{
  ArrayType ret{array.size()};
  Add(scalar, array, ret);
  return ret;
}
template <typename ArrayType>
meta::IsMathShapelessArrayLike<ArrayType, ArrayType> Add(ArrayType const &array1, ArrayType const &array2)
{
  ArrayType ret{array1.size()};
  Add(array1, array2, ret);
  return ret;
}
template <typename ArrayType>
meta::IsMathShapelessArrayLike<ArrayType, ArrayType> Add(ArrayType const &array1, ArrayType const &array2,
                         memory::Range const &range)
{
  ArrayType ret{array1.size()};
  Add(array1, array2, range, ret);
  return ret;
}

/**
 * Implementation for scalar addition. Implementing this helps keeps a uniform interface
 * @tparam T
 * @param scalar1
 * @param scalar2
 * @param ret
 */
template <typename S>
meta::IfIsArithmetic<S, void> Add(S const &scalar1, S const &scalar2, S &ret)
{
  ret = scalar1 + scalar2;
}
template <typename S>
meta::IfIsArithmetic<S, S> Add(S const &scalar1, S const &scalar2)
{
  S ret;
  Add(scalar1, scalar2, ret);
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
template <typename T, typename ArrayType>
meta::IsMathShapeArrayLike<ArrayType, void> Add(ArrayType const &array, T const &scalar, ArrayType &ret)
{
  assert(array.shape() == ret.shape());
  typename ArrayType::vector_register_type val(scalar);

  ret.data().in_parallel().Apply(
      [val](typename ArrayType::vector_register_type const &x,
            typename ArrayType::vector_register_type &      z) { z = x + val; },
      array.data());
}
template <typename T, typename ArrayType>
meta::IsMathShapeArrayLike<ArrayType, ArrayType> Add(ArrayType const &array, T const &scalar)
{
  ArrayType ret{array.shape()};
  Add(array, scalar, ret);
  return ret;
}
template <typename T, typename ArrayType>
meta::IsMathShapeArrayLike<ArrayType, void> Add(T const &scalar, ArrayType const &array, ArrayType &ret)
{
  ret = Add(array, scalar, ret);
}
template <typename T, typename ArrayType>
meta::IsMathShapeArrayLike<ArrayType, ArrayType> Add(T const &scalar, ArrayType const &array)
{
  ArrayType ret{array.shape()};
  Add(scalar, array, ret);
  return ret;
}
template <typename ArrayType>
fetch::math::meta::IsMathShapeArrayLike<ArrayType, void> Add(ArrayType const &array1, ArrayType const &array2, ArrayType &ret)
{
  assert(array1.shape() == array2.shape());
  assert(array1.shape() == ret.shape());

  memory::Range range{0, std::min(array1.data().size(), array2.data().size()), 1};
  Add(array1, array2, range, ret);
}
template <typename ArrayType>
fetch::math::meta::IsMathShapeArrayLike<ArrayType, ArrayType> Add(ArrayType const &array1, ArrayType const &array2)
{
  assert(array1.shape() == array2.shape());
  ArrayType ret{array1.shape()};
  Add(array1, array2, ret);

  return ret;
}



template <typename ArrayType>
meta::IsMathArrayLike<ArrayType, void> Add(ArrayType const &array1, ArrayType const &array2,
         memory::Range const &range, ArrayType &ret)
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
        [](typename ArrayType::vector_register_type const &x,
           typename ArrayType::vector_register_type const &y,
           typename ArrayType::vector_register_type &      z) { z = x + y; },
        array1.data(), array2.data());
  }
  else
  {
    TODO_FAIL_ROOT("Non-trivial ranges not implemented");
  }
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
  NDArray<T, C> ret{array1.shape()};
  Add(array1, array2, ret);
  return ret;
}

}  // namespace math
}  // namespace fetch