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

#include "math/meta/math_type_traits.hpp"

/**
 * assigns the absolute of x to this array
 * @param x
 */

namespace fetch {
namespace math {

namespace details {
template <typename ArrayType>
meta::IfIsMathArray<ArrayType, void> Add(ArrayType const &array1, ArrayType const &array2,
                                         memory::Range const &range, ArrayType &ret)
{
  assert(array1.size() == array2.size());
  assert(array1.size() == ret.size());
  //  ret.Reshape(array1.size());

  // TODO (private 516)
  assert(range.is_trivial() || range.is_undefined());

  if (range.is_undefined())
  {
    Add(array1, array2, ret);
  }
  else
  {
    auto r = range.ToTrivialRange(ret.data().size());

    ret.data().in_parallel().Apply(r,
                                   [](typename ArrayType::vector_register_type const &x,
                                      typename ArrayType::vector_register_type const &y,
                                      typename ArrayType::vector_register_type &z) { z = x + y; },
                                   array1.data(), array2.data());
  }
}

template <typename ArrayType>
meta::IfIsMathArray<ArrayType, ArrayType> Add(ArrayType const &array1, ArrayType const &array2,
                                              memory::Range const &range)
{
  ArrayType ret{array1.size()};
  Add(array1, array2, range, ret);
  return ret;
}

template <typename ArrayType>
meta::IfIsMathArray<ArrayType, void> Multiply(ArrayType const &obj1, ArrayType const &obj2,
                                              ArrayType &ret)
{
  assert(obj1.size() == obj2.size());
  for (std::size_t i = 0; i < ret.size(); ++i)
  {
    ret[i] = obj1[i] * obj2[i];
  }
}

template <typename ArrayType, typename T>
meta::IfIsMathArray<ArrayType, void> Subtract(ArrayType const &array1, ArrayType const &array2,
                                              ArrayType &ret)
{
  for (std::size_t i = 0; i < ret.size(); ++i)
  {
    ret.At(i) = array1 - array2.At(i);
  }
}

}  // namespace details

/////////////////
/// ADDITIONS ///
/////////////////

//////////////////
/// INTERFACES ///
//////////////////

template <typename S>
meta::IfIsArithmetic<S, S> Add(S const &scalar1, S const &scalar2)
{
  S ret;
  Add(scalar1, scalar2, ret);
  return ret;
}
template <typename S>
meta::IfIsFixedPoint<S, S> Add(S const &scalar1, S const &scalar2)
{
  S ret;
  Add(scalar1, scalar2, ret);
  return ret;
}

template <typename T, typename ArrayType>
meta::IfIsMathShapeArray<ArrayType, ArrayType> Add(ArrayType const &array, T const &scalar)
{
  ArrayType ret{array.shape()};
  Add(array, scalar, ret);
  return ret;
}
template <typename T, typename ArrayType>
meta::IfIsMathShapelessArray<ArrayType, ArrayType> Add(ArrayType const &array, T const &scalar)
{
  ArrayType ret{array.size()};
  Add(array, scalar, ret);
  return ret;
}
template <typename T, typename ArrayType>
meta::IfIsMathArray<ArrayType, ArrayType> Add(T const &scalar, ArrayType const &array)
{
  return Add(array, scalar);
}

template <typename T, typename ArrayType>
meta::IfIsMathArray<ArrayType, void> Add(T const &scalar, ArrayType const &array, ArrayType &ret)
{
  ret = Add(array, scalar, ret);
}
template <typename ArrayType>
meta::IfIsMathShapeArray<ArrayType, ArrayType> Add(ArrayType const &array1, ArrayType const &array2)
{
  assert(array1.shape() == array2.shape());
  ArrayType ret{array1.shape()};
  Add(array1, array2, ret);
  return ret;
}

///////////////////////
/// IMPLEMENTATIONS ///
///////////////////////

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
meta::IfIsFixedPoint<S, void> Add(S const &scalar1, S const &scalar2, S &ret)
{
  ret = scalar1 + scalar2;
}

template <typename T, typename ArrayType>
meta::IfIsNonBlasArray<ArrayType, void> Add(ArrayType const &array, T const &scalar, ArrayType &ret)
{
  assert(array.size() == ret.size());
  for (std::size_t i = 0; i < ret.size(); ++i)
  {
    ret.Set(i, array.At(i) + scalar);
  }
}

template <typename T, typename ArrayType>
meta::IfIsMathFixedPointArray<ArrayType, void> Add(ArrayType const &array, T const &scalar,
                                                   ArrayType &ret)
{
  assert(array.size() == ret.size());
  for (std::size_t i = 0; i < ret.size(); ++i)
  {
    ret.Set(i, array.At(i) + scalar);
  }
}

template <typename T, typename ArrayType>
meta::IfIsBlasArray<ArrayType, void> Add(ArrayType const &array, T const &scalar, ArrayType &ret)
{
  assert(array.size() == ret.size());
  typename ArrayType::vector_register_type val(scalar);

  ret.data().in_parallel().Apply(
      [val](typename ArrayType::vector_register_type const &x,
            typename ArrayType::vector_register_type &      z) { z = x + val; },
      array.data());
}

/**
 * Adds two arrays together
 * @tparam T
 * @tparam C
 * @param array1
 * @param array2
 * @param ret
 */
template <typename ArrayType>
meta::IfIsMathShapeArray<ArrayType, void> Add(ArrayType const &array1, ArrayType const &array2,
                                              ArrayType &ret)
{
  assert(array1.shape() == array2.shape());
  assert(array1.shape() == ret.shape());

  for (std::size_t i = 0; i < ret.size(); ++i)
  {
    ret.At(i) = array1.At(i) - array2.At(i);
  }
}

/////////////////////////////////////////////////////////////////
/// SHAPELESS ARRAY - SHAPELESS ARRAY  ADDITION - FIXED POINT ///
/////////////////////////////////////////////////////////////////

template <typename ArrayType>
meta::IfIsMathFixedPointShapelessArray<ArrayType, void> Add(ArrayType const &array,
                                                            ArrayType const &array2, ArrayType &ret)
{
  assert(array.size() == ret.size());
  for (std::size_t i = 0; i < ret.size(); ++i)
  {
    ret[i] = array[i] + array2[i];
  }
}

////////////////////////////////////////////////////////////////////
/// SHAPELESS ARRAY - SHAPELESS ARRAY  ADDITION - NO FIXED POINT ///
////////////////////////////////////////////////////////////////////

template <typename ArrayType>
meta::IfIsMathShapelessArray<ArrayType, ArrayType> Add(ArrayType const &array1,
                                                       ArrayType const &array2)
{
  assert(array1.size() == array2.size());
  ArrayType ret{array1.size()};
  Add(array1, array2, ret);
  return ret;
}
template <typename ArrayType>
meta::IfIsMathShapelessArray<ArrayType, ArrayType> Add(ArrayType const &    array1,
                                                       ArrayType const &    array2,
                                                       memory::Range const &range)
{
  assert(array1.size() == array2.size());
  ArrayType ret{array1.size()};
  details::Add(array1, array2, range, ret);
  return ret;
}
template <typename ArrayType>
meta::IfIsBlasArray<ArrayType, void> Add(ArrayType const &array1, ArrayType const &array2,
                                         ArrayType &ret)
{
  assert(array1.size() == array2.size());
  assert(array1.size() == ret.size());

  memory::Range range{0, std::min(array1.data().size(), array2.data().size()), 1};
  details::Add(array1, array2, range, ret);
}

//////////////////////////////////////
///// ARRAY BROADCASTING ADDITION  ///
//////////////////////////////////////
//
///**
// * Adds two ndarrays together with broadcasting
// * @tparam T
// * @tparam C
// * @param array1
// * @param array2
// * @param range
// * @param ret
// */
// template <typename T, typename C>
// void Add(NDArray<T, C> &array1, NDArray<T, C> &array2, NDArray<T, C> &ret)
//{
//  Broadcast([](T x, T y) { return x + y; }, array1, array2, ret);
//}
// template <typename T, typename C>
// NDArray<T, C> Add(NDArray<T, C> &array1, NDArray<T, C> &array2)
//{
//  NDArray<T, C> ret{array1.shape()};
//  Add(array1, array2, ret);
//  return ret;
//}

//////////////////////////
/// ADDITION OPERATORS ///
//////////////////////////

template <typename OtherType>
meta::IfIsMath<OtherType, void> operator+=(OtherType &left, OtherType const &right)
{
  Add(left, right, left);
}

///////////////////
/// SUBTRACTION ///
///////////////////

/////////////////
/// INTERFACE ///
/////////////////

template <typename ArrayType, typename T>
meta::IfIsMathShapeArray<ArrayType, ArrayType> Subtract(T const &scalar, ArrayType const &array)
{
  ArrayType ret{array.shape()};
  Subtract(scalar, array, ret);
  return ret;
}
template <typename ArrayType, typename T>
meta::IfIsMathShapeArray<ArrayType, ArrayType> Subtract(ArrayType const &array, T const &scalar)
{
  ArrayType ret{array.shape()};
  Subtract(array, scalar, ret);
  return ret;
}
template <typename T, typename ArrayType>
meta::IfIsMathShapelessArray<ArrayType, ArrayType> Subtract(ArrayType const &array, T const &scalar)
{
  ArrayType ret{array.size()};
  Subtract(array, scalar, ret);
  return ret;
}

template <typename T, typename C>
meta::IfIsMathShapelessArray<ShapelessArray<T, C>, ShapelessArray<T, C>> Subtract(
    T const &scalar, ShapelessArray<T, C> const &array)
{
  ShapelessArray<T, C> ret{array.size()};
  Subtract(scalar, array, ret);
  return ret;
}

template <typename ArrayType>
meta::IfIsBlasArray<ArrayType, void> Subtract(ArrayType const &obj1, ArrayType const &obj2,
                                              ArrayType &ret)
{
  memory::Range range{0, std::min(obj1.data().size(), obj1.data().size()), 1};
  Subtract(obj1, obj2, range, ret);
}
template <typename ArrayType>
meta::IfIsMathShapelessArray<ArrayType, void> Subtract(ArrayType const &obj1, ArrayType const &obj2)
{
  assert(obj1.size() == obj2.size());
  ArrayType ret{obj1.size()};
  Subtract(obj1, obj2, ret);
  return ret;
}
template <typename ArrayType>
meta::IfIsMathShapeArray<ArrayType, ArrayType> Subtract(ArrayType const &    obj1,
                                                        ArrayType const &    obj2,
                                                        memory::Range const &range)
{
  ArrayType ret{obj1.shape()};
  Subtract(obj1, obj2, range, ret);
  return ret;
}

//////////////////////
/// IMPLEMENTATION ///
//////////////////////

template <typename ArrayType, typename T>
meta::IfIsMathShapeArray<ArrayType, void> Subtract(T const &scalar, ArrayType const &array,
                                                   ArrayType &ret)
{
  assert(array.size() == ret.size());
  assert(array.shape() == ret.shape());
  for (std::size_t i = 0; i < ret.size(); ++i)
  {
    ret.At(i) = scalar - array.At(i);
  }
}
template <typename ArrayType, typename T>
meta::IfIsMathShapelessArray<ArrayType, void> Subtract(T const &scalar, ArrayType const &array,
                                                       ArrayType &ret)
{
  assert(array.size() == ret.size());
  for (std::size_t i = 0; i < ret.size(); ++i)
  {
    ret.At(i) = scalar - array.At(i);
  }
}

template <typename ArrayType, typename T>
meta::IfIsMathFixedPointArray<ArrayType, void> Subtract(ArrayType const &array, T const &scalar,
                                                        ArrayType &ret)
{
  assert(array.size() == ret.size());
  for (std::size_t i = 0; i < ret.size(); ++i)
  {
    ret.At(i) = array.At(i) - scalar;
  }
}
template <typename ArrayType, typename T>
meta::IfIsNonBlasArray<ArrayType, void> Subtract(ArrayType const &array, T const &scalar,
                                                 ArrayType &ret)
{
  assert(array.size() == ret.size());
  for (std::size_t i = 0; i < ret.size(); ++i)
  {
    ret.At(i) = array.At(i) - scalar;
  }
}

/**
 * subtract a scalar from every value in the array
 * @tparam T
 * @tparam C
 * @param array1
 * @param scalar
 * @param ret
 */
template <typename T, typename ArrayType>
meta::IfIsBlasArray<ArrayType, void> Subtract(ArrayType const &array, T const &scalar,
                                              ArrayType &ret)
{
  assert(array.size() == ret.size());
  assert(array.data().size() == ret.data().size());

  typename ArrayType::vector_register_type val(scalar);

  ret.data().in_parallel().Apply(
      [val](typename ArrayType::vector_register_type const &x,
            typename ArrayType::vector_register_type &      z) { z = x - val; },
      array.data());
}

template <typename T, typename C>
meta::IfIsMathShapelessArray<ShapelessArray<T, C>, void> Subtract(T const &scalar,
                                                                  ShapelessArray<T, C> const &array,
                                                                  ShapelessArray<T, C> &      ret)
{
  assert(array.size() == ret.size());
  for (std::size_t i = 0; i < ret.size(); ++i)
  {
    ret[i] = scalar - array[i];
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
template <typename ArrayType>
meta::IfIsBlasArray<ArrayType, void> Subtract(ArrayType const &obj1, ArrayType const &obj2,
                                              memory::Range const &range, ArrayType &ret)
{
  assert(obj1.size() == obj2.size());
  assert(obj1.size() == ret.size());

  assert(range.is_undefined() || range.is_trivial());

  if (range.is_undefined())
  {
    Subtract(obj1, obj2, ret);
  }
  else
  {
    auto r = range.ToTrivialRange(ret.data().size());

    ret.data().in_parallel().Apply(r,
                                   [](typename ArrayType::vector_register_type const &x,
                                      typename ArrayType::vector_register_type const &y,
                                      typename ArrayType::vector_register_type &z) { z = x - y; },
                                   obj1.data(), obj2.data());
  }
}

template <typename ArrayType>
meta::IfIsNonBlasArray<ArrayType, void> Subtract(ArrayType const &array, ArrayType const &array2,
                                                 ArrayType &ret)
{
  assert(array.size() == ret.size());
  for (std::size_t i = 0; i < ret.size(); ++i)
  {
    ret.Set(i, array.At(i) - array2.At(i));
  }
}

template <typename ArrayType>
meta::IfIsMathFixedPointArray<ArrayType, void> Subtract(ArrayType const &array,
                                                        ArrayType const &array2, ArrayType &ret)
{
  assert(array.size() == ret.size());
  for (std::size_t i = 0; i < ret.size(); ++i)
  {
    ret[i] = array[i] - array2[i];
  }
}
/////////////////////////////////////////
///// ARRAY BROADCASTING SUBTRACTION  ///
/////////////////////////////////////////
//
///**
// * subtract array from another array with broadcasting
// * @tparam T
// * @tparam C
// * @param array1
// * @param scalar
// * @param ret
// */
// template <typename T, typename C>
// void Subtract(NDArray<T, C> &obj1, NDArray<T, C> &obj2, NDArray<T, C> &ret)
//{
//  Broadcast([](T x, T y) { return x - y; }, obj1, obj2, ret);
//}
// template <typename T, typename C>
// NDArray<T, C> Subtract(NDArray<T, C> &obj1, NDArray<T, C> &obj2)
//{
//  assert(obj1.shape() == obj2.shape());
//  NDArray<T, C> ret{obj1.shape()};
//  Subtract(obj1, obj2, ret);
//  return ret;
//}

///////////////////////////////////////////////////////
/// SUBTRACTIONS - SCALAR & SCALAR - NO FIXED POINT ///
///////////////////////////////////////////////////////

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

//////////////////
/// INTERFACES ///
//////////////////

template <typename ArrayType, typename T>
meta::IfIsMathArray<ArrayType, void> Multiply(T const &scalar, ArrayType const &array,
                                              ArrayType &ret)
{
  Multiply(array, scalar, ret);
}
template <typename ArrayType, typename T>
meta::IfIsMathArray<ArrayType, ArrayType> Multiply(T const &scalar, ArrayType const &array)
{
  return Multiply(array, scalar);
}
template <typename ArrayType, typename T>
meta::IfIsMathShapeArray<ArrayType, ArrayType> Multiply(ArrayType const &array, T const &scalar)
{
  ArrayType ret{array.shape()};
  Multiply(array, scalar, ret);
  return ret;
}
template <typename ArrayType, typename T>
meta::IfIsMathShapelessArray<ArrayType, ArrayType> Multiply(ArrayType const &array, T const &scalar)
{
  ArrayType ret{array.size()};
  Multiply(array, scalar, ret);
  return ret;
}

template <typename ArrayType>
meta::IfIsMathShapelessArray<ArrayType, ArrayType> Multiply(ArrayType const &    obj1,
                                                            ArrayType const &    obj2,
                                                            memory::Range const &range)
{
  ArrayType ret{obj1.size()};
  Multiply(obj1, obj2, range, ret);
  return ret;
}
template <typename ArrayType>
meta::IfIsMathFixedPointArray<ArrayType, void> Multiply(ArrayType const &obj1,
                                                        ArrayType const &obj2, ArrayType &ret)
{
  details::Multiply(obj1, obj2, ret);
}
template <typename ArrayType>
meta::IfIsNonBlasArray<ArrayType, void> Multiply(ArrayType const &obj1, ArrayType const &obj2,
                                                 ArrayType &ret)
{
  details::Multiply(obj1, obj2, ret);
}

template <typename ArrayType>
meta::IfIsBlasArray<ArrayType, void> Multiply(ArrayType const &obj1, ArrayType const &obj2,
                                              ArrayType &ret)
{
  memory::Range range{0, std::min(obj1.data().size(), obj2.data().size()), 1};
  Multiply(obj1, obj2, range, ret);
}

template <typename ArrayType>
meta::IfIsMathShapelessArray<ArrayType, ArrayType> Multiply(ArrayType const &obj1,
                                                            ArrayType const &obj2)
{
  assert(obj1.size() == obj2.size());
  ArrayType ret{obj1.size()};
  Multiply(obj1, obj2, ret);
  return ret;
}
template <typename ArrayType>
meta::IfIsMathFixedPointArray<ArrayType, ArrayType> Multiply(ArrayType const &obj1,
                                                             ArrayType const &obj2)
{
  assert(obj1.size() == obj2.size());
  ArrayType ret{obj1.size()};
  Multiply(obj1, obj2, ret);
  return ret;
}

template <typename ArrayType>
meta::IfIsMathShapeArray<ArrayType, void> Multiply(ArrayType const &array1, ArrayType const &array2,
                                                   ArrayType &ret)
{
  memory::Range range{0, std::min(array1.data().size(), array2.data().size()), 1};
  Multiply(array1, array2, range, ret);
}
template <typename ArrayType>
meta::IfIsMathShapeArray<ArrayType, ArrayType> Multiply(ArrayType const &array1,
                                                        ArrayType const &array2)
{
  ArrayType ret{array1.shape()};
  Multiply(array1, array2, ret);
  return ret;
}
template <typename S>
meta::IfIsArithmetic<S, S> Multiply(S const &scalar1, S const &scalar2)
{
  S ret;
  Multiply(scalar1, scalar2, ret);
  return ret;
}

//////////////////////////////////
/// MULTIPLY - IMPLEMENTATIONS ///
//////////////////////////////////

/**
 * BLAS implementation of array & scalar multiplication
 * @tparam T
 * @tparam C
 * @param array1
 * @param scalar
 * @param ret
 */
template <typename ArrayType, typename T>
meta::IfIsBlasArray<ArrayType, void> Multiply(ArrayType const &array, T const &scalar,
                                              ArrayType &ret)
{
  assert(array.size() == ret.size());
  typename ArrayType::vector_register_type val(scalar);

  ret.data().in_parallel().Apply(
      [val](typename ArrayType::vector_register_type const &x,
            typename ArrayType::vector_register_type &      z) { z = x * val; },
      array.data());
}

template <typename ArrayType, typename T>
meta::IfIsNonBlasArray<ArrayType, void> Multiply(ArrayType const &array, T const &scalar,
                                                 ArrayType &ret)
{
  assert(array.size() == ret.size());
  for (std::size_t i = 0; i < ret.size(); ++i)
  {
    ret.Set(i, array.At(i) * scalar);
  }
}

template <typename ArrayType, typename T>
meta::IfIsMathFixedPointArray<ArrayType, void> Multiply(ArrayType const &array, T const &scalar,
                                                        ArrayType &ret)
{
  assert(array.size() == ret.size());
  for (std::size_t i = 0; i < ret.size(); ++i)
  {
    ret.Set(i, array.At(i) * scalar);
  }
}

/**
 * Multiply array by another array within a range
 * @tparam T
 * @tparam C
 * @param array1
 * @param scalar
 * @param ret
 */
template <typename ArrayType>
meta::IfIsMathShapelessArray<ArrayType, void> Multiply(ArrayType const &obj1, ArrayType const &obj2,
                                                       memory::Range const &range, ArrayType &ret)
{
  assert(obj1.size() == obj2.size());
  assert(obj1.size() == ret.size());

  // TODO (private 516)
  assert(range.is_trivial() || range.is_undefined());

  if (range.is_undefined())
  {
    Multiply(obj1, obj2, ret);
  }
  else
  {
    auto r = range.ToTrivialRange(ret.data().size());

    ret.data().in_parallel().Apply(r,
                                   [](typename ArrayType::vector_register_type const &x,
                                      typename ArrayType::vector_register_type const &y,
                                      typename ArrayType::vector_register_type &z) { z = x * y; },
                                   obj1.data(), obj2.data());
  }
}

template <typename ArrayType>
meta::IfIsMathShapeArray<ArrayType, void> Multiply(ArrayType const &obj1, ArrayType const &obj2,
                                                   memory::Range const &range, ArrayType &ret)
{
  assert(obj1.size() == obj2.size());
  assert(obj1.size() == ret.size());

  // TODO (private 516)
  assert(range.is_trivial() || range.is_undefined());

  if (range.is_undefined())
  {
    Multiply(obj1, obj2, ret);
  }
  else
  {
    auto r = range.ToTrivialRange(ret.data().size());

    ret.data().in_parallel().Apply(r,
                                   [](typename ArrayType::vector_register_type const &x,
                                      typename ArrayType::vector_register_type const &y,
                                      typename ArrayType::vector_register_type &z) { z = x * y; },
                                   obj1.data(), obj2.data());
  }
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

//////////////
/// DIVIDE ///
//////////////

/////////////////
/// INTERFACE ///
/////////////////

template <typename ArrayType, typename T>
meta::IfIsMathShapeArray<ArrayType, ArrayType> Divide(ArrayType const &array, T const &scalar)
{
  ArrayType ret{array.shape()};
  Divide(array, scalar, ret);
  return ret;
}

template <typename ArrayType>
meta::IfIsMathShapelessArray<ArrayType, ArrayType> Divide(ArrayType const &    obj1,
                                                          ArrayType const &    obj2,
                                                          memory::Range const &range)
{
  ArrayType ret{obj1.size()};
  Divide(obj1, obj2, range, ret);
  return ret;
}

template <typename ArrayType, typename T>
meta::IfIsMathShapelessArray<ArrayType, ArrayType> Divide(ArrayType const &array, T const &scalar)
{
  ArrayType ret{array.size()};
  Divide(array, scalar, ret);
  return ret;
}

template <typename ArrayType, typename T>
meta::IfIsMathShapelessArray<ArrayType, ArrayType> Divide(T const &scalar, ArrayType const &array)
{
  ArrayType ret{array.size()};
  Divide(scalar, array, ret);
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
template <typename ArrayType>
meta::IfIsBlasArray<ArrayType, void> Divide(ArrayType const &obj1, ArrayType const &obj2,
                                            ArrayType &ret)
{
  memory::Range range{0, std::min(obj1.data().size(), obj1.data().size()), 1};
  Divide(obj1, obj2, range, ret);
}
template <typename ArrayType>
meta::IfIsMathShapelessArray<ArrayType, ArrayType> Divide(ArrayType const &obj1,
                                                          ArrayType const &obj2)
{
  ArrayType ret{obj1.size()};
  Divide(obj1, obj2, ret);
  return ret;
}
//////////////////////
/// IMPLEMENTATION ///
//////////////////////

/**
 * divide array by a scalar
 * @tparam T
 * @tparam C
 * @param array1
 * @param scalar
 * @param ret
 */
template <typename ArrayType, typename T>
meta::IfIsBlasArray<ArrayType, void> Divide(ArrayType const &array, T const &scalar, ArrayType &ret)
{
  assert(array.size() == ret.size());
  typename ArrayType::vector_register_type val(scalar);

  ret.data().in_parallel().Apply(
      [val](typename ArrayType::vector_register_type const &x,
            typename ArrayType::vector_register_type &      z) { z = x / val; },
      array.data());
}
/**
 * elementwise divide scalar by array element
 * @tparam T
 * @tparam C
 * @param scalar
 * @param array
 * @param ret
 */
template <typename ArrayType, typename T>
meta::IfIsBlasArray<ArrayType, void> Divide(T const &scalar, ArrayType const &array, ArrayType &ret)
{
  assert(array.size() == ret.size());
  typename ArrayType::vector_register_type val(scalar);

  ret.data().in_parallel().Apply(
      [val](typename ArrayType::vector_register_type const &x,
            typename ArrayType::vector_register_type &      z) { z = val / x; },
      array.data());
}

template <typename ArrayType, typename T>
meta::IfIsNonBlasArray<ArrayType, void> Divide(ArrayType const &array, T const &scalar,
                                               ArrayType &ret)
{
  assert(array.size() == ret.size());
  for (std::size_t i = 0; i < ret.size(); ++i)
  {
    ret.At(i) = array.At(i) / scalar;
  }
}
template <typename ArrayType, typename T>
meta::IfIsMathFixedPointArray<ArrayType, void> Divide(ArrayType const &array, T const &scalar,
                                                      ArrayType &ret)
{
  assert(array.size() == ret.size());
  for (std::size_t i = 0; i < ret.size(); ++i)
  {
    ret.At(i) = array.At(i) / scalar;
  }
}
template <typename ArrayType, typename T>
meta::IfIsNonBlasArray<ArrayType, void> Divide(T const &scalar, ArrayType const &array,
                                               ArrayType &ret)
{
  assert(array.size() == ret.size());
  for (std::size_t i = 0; i < ret.size(); ++i)
  {
    ret.At(i) = scalar / array.At(i);
  }
}
template <typename ArrayType, typename T>
meta::IfIsMathFixedPointArray<ArrayType, void> Divide(T const &scalar, ArrayType const &array,
                                                      ArrayType &ret)
{
  assert(array.size() == ret.size());
  for (std::size_t i = 0; i < ret.size(); ++i)
  {
    ret.At(i) = scalar / array.At(i);
  }
}

template <typename ArrayType>
meta::IfIsBlasArray<ArrayType, void> Divide(ArrayType const &obj1, ArrayType const &obj2,
                                            memory::Range const &range, ArrayType &ret)
{
  assert(obj1.size() == obj2.size());
  assert(obj1.size() == ret.size());

  // TODO (private 516)
  assert(range.is_trivial() || range.is_undefined());

  if (obj1.size() == obj2.size())
  {
    if (range.is_undefined())
    {
      Divide(obj1, obj2, ret);
    }
    else
    {
      auto r = range.ToTrivialRange(ret.data().size());

      ret.data().in_parallel().Apply(r,
                                     [](typename ArrayType::vector_register_type const &x,
                                        typename ArrayType::vector_register_type const &y,
                                        typename ArrayType::vector_register_type &z) { z = x / y; },
                                     obj1.data(), obj2.data());
    }
  }
}
template <typename ArrayType>
meta::IfIsMathShapeArray<ArrayType, void> Divide(ArrayType const &obj1, ArrayType const &obj2,
                                                 ArrayType &ret)
{
  memory::Range range{0, std::min(obj1.data().size(), obj1.data().size()), 1};
  Divide(obj1, obj2, range, ret);
}
template <typename ArrayType>
meta::IfIsMathShapeArray<ArrayType, void> Divide(ArrayType const &obj1, ArrayType const &obj2)
{
  ArrayType ret{obj1.shape()};

  Divide(obj1, obj2, ret);
  return ret;
}

template <typename ArrayType, typename T>
meta::IfIsMathShapeArray<ArrayType, ArrayType> Divide(T const &scalar, ArrayType const &array)
{
  ArrayType ret{array.shape()};
  Divide(scalar, array, ret);
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
template <typename ArrayType>
meta::IfIsMathFixedPointArray<ArrayType, void> Divide(ArrayType const &obj1, ArrayType const &obj2,
                                                      ArrayType &ret)
{
  assert(obj1.size() == obj2.size());
  for (std::size_t i = 0; i < ret.size(); ++i)
  {
    ret[i] = obj1[i] / obj2[i];
  }
}
template <typename ArrayType>
meta::IfIsMathFixedPointArray<ArrayType, ArrayType> Divide(ArrayType const &obj1,
                                                           ArrayType const &obj2)
{
  assert(obj1.size() == obj2.size());
  ArrayType ret{obj1.size()};
  for (std::size_t i = 0; i < ret.size(); ++i)
  {
    ret[i] = obj1[i] / obj2[i];
  }
  return ret;
}
//
///**
// * subtract array from another array with broadcasting
// * @tparam T
// * @tparam C
// * @param array1
// * @param scalar
// * @param ret
// */
// template <typename T, typename C>
// void Divide(NDArray<T, C> &obj1, NDArray<T, C> &obj2, NDArray<T, C> &ret)
//{
//  Broadcast([](T x, T y) { return x / y; }, obj1, obj2, ret);
//}
// template <typename T, typename C>
// NDArray<T, C> Divide(NDArray<T, C> &obj1, NDArray<T, C> &obj2)
//{
//  assert(obj1.shape() == obj2.shape());
//  NDArray<T, C> ret{obj1.shape()};
//  Divide(obj1, obj2, ret);
//  return ret;
//}
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
