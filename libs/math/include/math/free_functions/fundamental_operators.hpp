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

/////////////////
/// ADDITIONS ///
/////////////////

namespace details {
template <typename ArrayType>
meta::IsMathArrayLike<ArrayType, void> Add(ArrayType const &array1, ArrayType const &array2,
                                           memory::Range const &range, ArrayType &ret)
{
  assert(array1.size() == array2.size());
  assert(array1.size() == ret.size());
  //  ret.Reshape(array1.size());

  if (range.is_undefined())
  {
    Add(array1, array2, ret);
  }
  else if (range.is_trivial())
  {
    auto r = range.ToTrivialRange(ret.data().size());

    ret.data().in_parallel().Apply(r,
                                   [](typename ArrayType::vector_register_type const &x,
                                      typename ArrayType::vector_register_type const &y,
                                      typename ArrayType::vector_register_type &z) { z = x + y; },
                                   array1.data(), array2.data());
  }
  else
  {
    TODO_FAIL_ROOT("Non-trivial ranges not implemented");
  }
}
template <typename ArrayType>
meta::IsMathArrayLike<ArrayType, ArrayType> Add(ArrayType const &array1, ArrayType const &array2,
                                                memory::Range const &range)
{
  ArrayType ret{array1.size()};
  Add(array1, array2, range, ret);
  return ret;
}

}  // namespace details

////////////////////////////////
/// SCALAR - SCALAR ADDITION ///
////////////////////////////////
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

//////////////////////////////////////
/// SHAPED ARRAY - SCALAR ADDITION ///
//////////////////////////////////////

template <typename T, typename ArrayType>
meta::IsMathShapeArrayLike<ArrayType, void> Add(ArrayType const &array, T const &scalar,
                                                ArrayType &ret)
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
meta::IsMathShapeArrayLike<ArrayType, void> Add(T const &scalar, ArrayType const &array,
                                                ArrayType &ret)
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

/////////////////////////////////////////
/// SHAPELESS ARRAY - SCALAR ADDITION ///
/////////////////////////////////////////

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
  ShapeLessArray<T, C> ret{array.size()};
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
  ShapeLessArray<T, C> ret{array.size()};
  Add(scalar, array, ret);
  return ret;
}

///////////////////////////////////////////////
/// SHAPED ARRAY - SHAPED ARRAY  ADDITION   ///
///////////////////////////////////////////////

/**
 * Adds two arrays together
 * @tparam T
 * @tparam C
 * @param array1
 * @param array2
 * @param ret
 */
template <typename ArrayType>
meta::IsMathShapeArrayLike<ArrayType, void> Add(ArrayType const &array1, ArrayType const &array2,
                                                ArrayType &ret)
{
  assert(array1.shape() == array2.shape());
  assert(array1.shape() == ret.shape());

  memory::Range range{0, std::min(array1.data().size(), array2.data().size()), 1};
  details::Add(array1, array2, range, ret);
}
template <typename ArrayType>
meta::IsMathShapeArrayLike<ArrayType, ArrayType> Add(ArrayType const &array1,
                                                     ArrayType const &array2)
{
  assert(array1.shape() == array2.shape());
  ArrayType ret{array1.shape()};
  details::Add(array1, array2, ret);

  return ret;
}

////////////////////////////////////////////////////
/// SHAPELESS ARRAY - SHAPELESS ARRAY  ADDITION  ///
////////////////////////////////////////////////////

template <typename ArrayType>
meta::IsMathShapelessArrayLike<ArrayType, ArrayType> Add(ArrayType const &array1,
                                                         ArrayType const &array2)
{
  assert(array1.size() == array2.size());
  ArrayType ret{array1.size()};
  Add(array1, array2, ret);
  return ret;
}
template <typename ArrayType>
meta::IsMathShapelessArrayLike<ArrayType, ArrayType> Add(ArrayType const &    array1,
                                                         ArrayType const &    array2,
                                                         memory::Range const &range)
{
  assert(array1.size() == array2.size());
  ArrayType ret{array1.size()};
  details::Add(array1, array2, range, ret);
  return ret;
}
template <typename ArrayType>
meta::IsMathShapelessArrayLike<ArrayType, void> Add(ArrayType const &array1,
                                                    ArrayType const &array2, ArrayType &ret)
{
  assert(array1.size() == array2.size());
  assert(array1.size() == ret.size());

  memory::Range range{0, std::min(array1.data().size(), array2.data().size()), 1};
  details::Add(array1, array2, range, ret);
}

////////////////////////////////////
/// ARRAY BROADCASTING ADDITION  ///
////////////////////////////////////

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

//////////////////////////
/// ADDITION OPERATORS ///
//////////////////////////

template <typename OtherType>
meta::IsMathLike<OtherType, void> operator+=(OtherType &left, OtherType const &right)
{
  Add(left, right, left);
}

////////////////////
/// SUBTRACTIONS ///
////////////////////

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
  ShapeLessArray<T, C> ret{array.size()};
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
  ShapeLessArray<T, C> ret{array.size()};
  Subtract(scalar, array, ret);
  return ret;
}

template <typename T, typename C, typename S>
linalg::Matrix<T, C, S> Subtract(T const &scalar, linalg::Matrix<T, C, S> const &array)
{
  linalg::Matrix<T, C, S> ret{array.shape()};
  Subtract(scalar, array, ret);
  return ret;
}
template <typename T, typename C, typename S>
void Subtract(T const &scalar, linalg::Matrix<T, C, S> const &array, linalg::Matrix<T, C, S> &ret)
{
  assert(array.size() == ret.size());
  assert(array.shape() == ret.shape());
  for (std::size_t i = 0; i < ret.size(); ++i)
  {
    ret[i] = scalar - array[i];
  }
}
template <typename T, typename C, typename S>
linalg::Matrix<T, C, S> Subtract(linalg::Matrix<T, C, S> const &array, T const &scalar)
{
  linalg::Matrix<T, C, S> ret{array.shape()};
  Subtract(array, scalar, ret);
  return ret;
}
template <typename T, typename C, typename S>
void Subtract(linalg::Matrix<T, C, S> const &array, T const &scalar, linalg::Matrix<T, C, S> &ret)
{
  assert(array.size() == ret.size());
  for (std::size_t i = 0; i < ret.size(); ++i)
  {
    ret[i] = array[i] - scalar;
  }
}
template <typename T, typename C, typename S>
linalg::Matrix<T, C, S> Subtract(linalg::Matrix<T, C, S> const &array1,
                                 linalg::Matrix<T, C, S> const &array2)
{
  linalg::Matrix<T, C, S> ret{array1.shape()};
  Subtract(array1, array2, ret);
  return ret;
}
template <typename T, typename C, typename S>
void Subtract(linalg::Matrix<T, C, S> const &array1, linalg::Matrix<T, C, S> const &array2,
              linalg::Matrix<T, C, S> &ret)
{
  // broadcasting is permissible
  assert((array1.size() == ret.size()) || (array1.shape()[0] == ret.shape()[0]) ||
         (array1.shape()[1] == ret.shape()[1]));
  assert((array1.size() == array2.size()) || (array1.shape()[0] == array2.shape()[0]) ||
         (array1.shape()[1] == array2.shape()[1]));

  if (array1.size() == array2.size())
  {
    for (std::size_t i = 0; i < ret.size(); ++i)
    {
      ret[i] = array1[i] - array2[i];
    }
  }
  else if (array1.shape()[0] == array2.shape()[0])
  {
    for (std::size_t i = 0; i < ret.shape()[0]; ++i)
    {
      for (std::size_t j = 0; j < ret.shape()[1]; ++j)
      {
        ret.Set(i, j, array1.At(i, j) - array2.At(i, 0));
      }
    }
  }
  else
  {
    for (std::size_t i = 0; i < ret.shape()[1]; ++i)
    {
      for (std::size_t j = 0; j < ret.shape()[0]; ++j)
      {
        ret.Set(j, i, array1.At(j, i) - array2.At(0, i));
      }
    }
  }
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
    TODO_FAIL_ROOT("Non-trivial ranges not implemented");
  }
}
template <typename T, typename C>
ShapeLessArray<T, C> Subtract(ShapeLessArray<T, C> const &obj1, ShapeLessArray<T, C> const &obj2,
                              memory::Range const &range)
{
  ShapeLessArray<T, C> ret{obj1.shape()};
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
  assert(obj1.size() == obj2.size());
  ShapeLessArray<T, C> ret{obj1.size()};
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
  assert(obj1.shape() == obj2.shape());
  NDArray<T, C> ret{obj1.shape()};
  Subtract(obj1, obj2, ret);
  return ret;
}
/**
 * Implementation for scalar subtraction. Implementing this helps keeps a uniform interface
 * @tparam T
 * @param scalar1
 * @param scalar2
 * @param ret
 */
template <typename S>
fetch::meta::IfIsArithmetic<S, void> Subtract(S const &scalar1, S const &scalar2, S &ret)
{
  ret = scalar1 - scalar2;
}
template <typename S>
fetch::meta::IfIsArithmetic<S, S> Subtract(S const &scalar1, S const &scalar2)
{
  S ret;
  Subtract(scalar1, scalar2, ret);
  return ret;
}

////////////////
/// Multiply ///
////////////////

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
  ShapeLessArray<T, C> ret{array.size()};
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
  ShapeLessArray<T, C> ret{array.size()};
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
    TODO_FAIL_ROOT("Non-trivial ranges not implemented");
  }
}
template <typename T, typename C>
ShapeLessArray<T, C> Multiply(ShapeLessArray<T, C> const &obj1, ShapeLessArray<T, C> const &obj2,
                              memory::Range const &range)
{
  ShapeLessArray<T, C> ret{obj1.size()};
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
  ShapeLessArray<T, C> ret{obj1.size()};
  Multiply(obj1, obj2, ret);
  return ret;
}

template <typename T, typename C, typename S>
void Multiply(linalg::Matrix<T, C, S> const &obj1, linalg::Matrix<T, C, S> const &obj2,
              memory::Range const &range, linalg::Matrix<T, C, S> &ret)
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
        [](typename linalg::Matrix<T, C, S>::vector_register_type const &x,
           typename linalg::Matrix<T, C, S>::vector_register_type const &y,
           typename linalg::Matrix<T, C, S>::vector_register_type &      z) { z = x * y; },
        obj1.data(), obj2.data());
  }
  else
  {
    TODO_FAIL_ROOT("Non-trivial ranges not implemented");
  }
}
template <typename T, typename C, typename S>
void Multiply(linalg::Matrix<T, C, S> const &array1, linalg::Matrix<T, C, S> const &array2,
              linalg::Matrix<T, C, S> &ret)
{
  memory::Range range{0, std::min(array1.data().size(), array2.data().size()), 1};
  Multiply(array1, array2, range, ret);
}
template <typename T, typename C, typename S>
linalg::Matrix<T, C, S> Multiply(linalg::Matrix<T, C, S> const &array1,
                                 linalg::Matrix<T, C, S> const &array2)
{
  linalg::Matrix<T, C, S> ret{array1.shape()};
  Multiply(array1, array2, ret);
  return ret;
}
template <typename T, typename C, typename S>
void Multiply(linalg::Matrix<T, C, S> const &array, T const &scalar, linalg::Matrix<T, C, S> &ret)
{
  assert(array.size() == ret.size());
  typename linalg::Matrix<T, C, S>::vector_register_type val(scalar);

  ret.data().in_parallel().Apply(
      [val](typename linalg::Matrix<T, C, S>::vector_register_type const &x,
            typename linalg::Matrix<T, C, S>::vector_register_type &      z) { z = x * val; },
      array.data());
}
template <typename T, typename C, typename S>
linalg::Matrix<T, C, S> Multiply(linalg::Matrix<T, C, S> const &array, T const &scalar)
{
  linalg::Matrix<T, C, S> ret{array.shape()};
  Multiply(array, scalar, ret);
  return ret;
}
template <typename T, typename C, typename S>
void Multiply(T const &scalar, linalg::Matrix<T, C, S> const &array, linalg::Matrix<T, C, S> &ret)
{
  Multiply(array, scalar, ret);
}
template <typename T, typename C, typename S>
linalg::Matrix<T, C, S> Multiply(T const &scalar, linalg::Matrix<T, C, S> const &array)
{
  linalg::Matrix<T, C, S> ret{array.shape()};
  Multiply(scalar, array, ret);
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
  NDArray<T, C> ret{obj1.shape()};
  Multiply(obj1, obj2, ret);
  return ret;
}
/**
 * Implementation for scalar multiplication. Implementing this helps keeps a uniform interface
 * @tparam T
 * @param scalar1
 * @param scalar2
 * @param ret
 */
template <typename S>
meta::IfIsArithmetic<S, void> Multiply(S const &scalar1, S const &scalar2, S &ret)
{
  ret = scalar1 * scalar2;
}
template <typename S>
meta::IfIsArithmetic<S, S> Multiply(S const &scalar1, S const &scalar2)
{
  S ret;
  Multiply(scalar1, scalar2, ret);
  return ret;
}

//////////////
/// DIVIDE ///
//////////////

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
  ShapeLessArray<T, C> ret{array.size()};
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
  ShapeLessArray<T, C> ret{array.size()};
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
    TODO_FAIL_ROOT("Non-trivial ranges not implemented");
  }
}
template <typename T, typename C>
ShapeLessArray<T, C> Divide(ShapeLessArray<T, C> const &obj1, ShapeLessArray<T, C> const &obj2,
                            memory::Range const &range)
{
  ShapeLessArray<T, C> ret{obj1.size()};
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
  ShapeLessArray<T, C> ret{obj1.size()};
  Divide(obj1, obj2, ret);
  return ret;
}

template <typename T, typename C, typename S>
void Divide(linalg::Matrix<T, C, S> const &obj1, linalg::Matrix<T, C, S> const &obj2,
            memory::Range const &range, linalg::Matrix<T, C, S> &ret)
{
  assert((obj1.size() == obj2.size()) || (obj1.shape()[0] == obj2.shape()[0]) ||
         (obj1.shape()[1] == obj2.shape()[1]));
  assert(obj1.size() == ret.size());

  if (obj1.size() == obj2.size())
  {
    if (range.is_undefined())
    {
      Divide(obj1, obj2, ret);
    }
    else if (range.is_trivial())
    {
      auto r = range.ToTrivialRange(ret.data().size());

      ret.data().in_parallel().Apply(
          r,
          [](typename linalg::Matrix<T, C, S>::vector_register_type const &x,
             typename linalg::Matrix<T, C, S>::vector_register_type const &y,
             typename linalg::Matrix<T, C, S>::vector_register_type &      z) { z = x / y; },
          obj1.data(), obj2.data());
    }
    else
    {
      TODO_FAIL_ROOT("Non-trivial ranges not implemented");
    }
  }
  else if (obj1.shape()[0] == obj2.shape()[0])
  {
    assert(obj2.shape()[1] == 1);
    for (std::size_t i = 0; i < obj1.shape()[0]; ++i)
    {
      for (std::size_t j = 0; j < obj1.shape()[1]; ++j)
      {
        ret.Set(i, j, obj1.At(i, j) / obj2.At(i, 0));
      }
    }
  }
  else
  {
    assert(obj2.shape()[0] == 1);
    for (std::size_t i = 0; i < obj1.shape()[0]; ++i)
    {
      for (std::size_t j = 0; j < obj1.shape()[1]; ++j)
      {
        ret.Set(i, j, obj1.At(i, j) / obj2.At(0, j));
      }
    }
  }
}
template <typename T, typename C, typename S>
void Divide(linalg::Matrix<T, C, S> const &obj1, linalg::Matrix<T, C, S> const &obj2,
            linalg::Matrix<T, C, S> &ret)
{
  memory::Range range{0, std::min(obj1.data().size(), obj1.data().size()), 1};
  Divide(obj1, obj2, range, ret);
}
template <typename T, typename C, typename S>
linalg::Matrix<T, C, S> Divide(linalg::Matrix<T, C, S> const &obj1,
                               linalg::Matrix<T, C, S> const &obj2)
{
  linalg::Matrix<T, C, S> ret{obj1.shape()};

  Divide(obj1, obj2, ret);
  return ret;
}
template <typename T, typename C, typename S>
void Divide(linalg::Matrix<T, C, S> const &array, T const &scalar, linalg::Matrix<T, C, S> &ret)
{
  assert(array.size() == ret.size());
  typename linalg::Matrix<T, C, S>::vector_register_type val(scalar);

  ret.data().in_parallel().Apply(
      [val](typename linalg::Matrix<T, C, S>::vector_register_type const &x,
            typename linalg::Matrix<T, C, S>::vector_register_type &      z) { z = x / val; },
      array.data());
}
template <typename T, typename C, typename S>
linalg::Matrix<T, C, S> Divide(linalg::Matrix<T, C, S> const &array, T const &scalar)
{
  linalg::Matrix<T, C, S> ret{array.shape()};
  Divide(array, scalar, ret);
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
  assert(obj1.shape() == obj2.shape());
  NDArray<T, C> ret{obj1.shape()};
  Divide(obj1, obj2, ret);
  return ret;
}
/**
 * Implementation for scalar division. Implementing this helps keeps a uniform interface
 * @tparam T
 * @param scalar1
 * @param scalar2
 * @param ret
 */
template <typename S>
meta::IfIsArithmetic<S, void> Divide(S const &scalar1, S const &scalar2, S &ret)
{
  ret = scalar1 / scalar2;
}
template <typename S>
meta::IfIsArithmetic<S, S> Divide(S const &scalar1, S const &scalar2)
{
  S ret;
  Divide(scalar1, scalar2, ret);
  return ret;
}

}  // namespace math
}  // namespace fetch
