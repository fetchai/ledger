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

#include "core/assert.hpp"
#include "math/meta/math_type_traits.hpp"

namespace fetch {
namespace math {

namespace details_vectorisation {

// TODO(private 854): vectorisation implementations not yet called

/**
 * Elementwise Addition over a specified range within two arrays - vectorised implementation
 * @tparam ArrayType
 * @param array1
 * @param array2
 * @param range
 * @param ret
 * @return
 */
template <typename ArrayType>
meta::IfIsMathArray<ArrayType, void> Add(ArrayType const &array1, ArrayType const &array2,
                                         memory::Range const &range, ArrayType &ret)
{
  using VectorRegisterType = typename ArrayType::VectorRegisterType;
  ASSERT(array1.size() == array2.size());
  ASSERT(array1.size() == ret.size());
  //  ret.Reshape(array1.size());

  // TODO (private 516)
  ASSERT(range.is_trivial() || range.is_undefined());

  if (range.is_undefined())
  {
    Add(array1, array2, ret);
  }
  else
  {
    auto r = range.ToTrivialRange(ret.data().size());

    ret.data().in_parallel().Apply(
        r,
        [](VectorRegisterType const &x, VectorRegisterType const &y, VectorRegisterType &z) {
          z = x + y;
        },
        array1.data(), array2.data());
  }
}

/**
 * Interface to elementwise array addition over a range
 * @tparam ArrayType
 * @param array1
 * @param array2
 * @param range
 * @return
 */
template <typename ArrayType>
meta::IfIsMathArray<ArrayType, ArrayType> Add(ArrayType const &array1, ArrayType const &array2,
                                              memory::Range const &range)
{
  ArrayType ret{array1.size()};
  Add(array1, array2, range, ret);
  return ret;
}

/**
 * Subtract a scalar from every value in the array
 * @tparam data type
 * @tparam ArrayType Tensor/Array type
 * @param array input array
 * @param scalar value to subtract elementwise
 * @param ret return array storing result
 */
template <typename T, typename ArrayType,
          typename = std::enable_if_t<fetch::math::meta::IsArithmetic<T>>>
meta::IfIsMathArray<ArrayType, void> Subtract(ArrayType const &array, T const &scalar,
                                              ArrayType &ret)
{
  using VectorRegisterType = typename ArrayType::VectorRegisterType;
  ASSERT(array.size() == ret.size());
  ASSERT(array.data().size() == ret.data().size());

  VectorRegisterType val(scalar);

  ret.data().in_parallel().Apply(
      [val](VectorRegisterType const &x, VectorRegisterType &z) { z = x - val; }, array.data());
}

/**
 * Multiply array by another array elementwise within a specified range
 * @tparam ArrayType Array or Tensor type
 * @param obj1 Array 1
 * @param obj2 Array 2
 * @param range The range over which to multiply elements
 * @param ret The return array storing the result
 */
template <typename ArrayType>
meta::IfIsMathArray<ArrayType, void> Multiply(ArrayType const &obj1, ArrayType const &obj2,
                                              memory::Range const &range, ArrayType &ret)
{
  using VectorRegisterType = typename ArrayType::VectorRegisterType;
  ASSERT(obj1.shape() == obj2.shape());
  ASSERT(obj1.shape() == ret.shape());

  // TODO (private 516)
  ASSERT(range.is_trivial() || range.is_undefined());

  if (range.is_undefined())
  {
    Multiply(obj1, obj2, ret);
  }
  else
  {
    auto r = range.ToTrivialRange(ret.data().size());

    ret.data().in_parallel().Apply(
        r,
        [](VectorRegisterType const &x, VectorRegisterType const &y, VectorRegisterType &z) {
          z = x * y;
        },
        obj1.data(), obj2.data());
  }
}

/**
 * Subtract array from another array elementwise within a specified range
 * @tparam ArrayType Array or Tensor type
 * @param obj1  Array1
 * @param obj2  Array2
 * @param range  Range over which to perform subtractions
 * @param ret  the return array storing the result
 * @return
 */
template <typename ArrayType>
meta::IfIsMathArray<ArrayType, void> Subtract(ArrayType const &obj1, ArrayType const &obj2,
                                              memory::Range const &range, ArrayType &ret)
{
  using VectorRegisterType = typename ArrayType::VectorRegisterType;
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

    ret.data().in_parallel().Apply(
        r,
        [](VectorRegisterType const &x, VectorRegisterType const &y, VectorRegisterType &z) {
          z = x - y;
        },
        obj1.data(), obj2.data());
  }
}

/**
 * Multiply every element in an array by a scalar
 * @tparam ArrayType type of array or tensor
 * @tparam T scalar data type
 * @param array the array
 * @param scalar the scalar
 * @param ret return array storing the result
 * @return
 */
template <typename ArrayType, typename T,
          typename = std::enable_if_t<fetch::math::meta::IsArithmetic<T>>>
meta::IfIsMathArray<ArrayType, void> Multiply(ArrayType const &array, T const &scalar,
                                              ArrayType &ret)
{
  using VectorRegisterType = typename ArrayType::VectorRegisterType;
  assert(array.size() == ret.size());
  VectorRegisterType val(scalar);

  ret.data().in_parallel().Apply(
      [val](VectorRegisterType const &x, VectorRegisterType &z) { z = x * val; }, array.data());
}

/**
 * Divide array by scalar elementwise
 * @tparam ArrayType The Array type
 * @tparam T The scalar type
 * @param array The array
 * @param scalar The scalar
 * @param ret the return array storing the result
 */
template <typename ArrayType, typename T>
meta::IfIsMathArray<ArrayType, void> Divide(ArrayType const &array, T const &scalar, ArrayType &ret)
{
  using VectorRegisterType = typename ArrayType::VectorRegisterType;

  assert(array.size() == ret.size());
  VectorRegisterType val(scalar);

  ret.data().in_parallel().Apply(
      [val](VectorRegisterType const &x, VectorRegisterType &z) { z = x / val; }, array.data());
}

/**
 * Divide scalar by array values elementwise
 * @tparam ArrayType The Array type
 * @tparam T The scalar type
 * @param scalar The scalar
 * @param array The array
 * @param ret the return array storing the result
 */
template <typename ArrayType, typename T>
meta::IfIsMathArray<ArrayType, void> Divide(T const &scalar, ArrayType const &array, ArrayType &ret)
{
  using VectorRegisterType = typename ArrayType::VectorRegisterType;

  assert(array.size() == ret.size());
  VectorRegisterType val(scalar);

  ret.data().in_parallel().Apply(
      [val](VectorRegisterType const &x, VectorRegisterType &z) { z = val / x; }, array.data());
}

/**
 * Divide array1 by array2 elementwise
 * @tparam ArrayType The Array type
 * @param obj1 array1
 * @param obj2 array2
 * @param range The range over which to compute divisions
 * @param ret the return array storing the result
 */
template <typename ArrayType>
meta::IfIsMathArray<ArrayType, void> Divide(ArrayType const &obj1, ArrayType const &obj2,
                                            memory::Range const &range, ArrayType &ret)
{
  using VectorRegisterType = typename ArrayType::VectorRegisterType;
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

      ret.data().in_parallel().Apply(
          r,
          [](VectorRegisterType const &x, VectorRegisterType const &y, VectorRegisterType &z) {
            z = x / y;
          },
          obj1.data(), obj2.data());
    }
  }
}

}  // namespace details_vectorisation

namespace implementations {

////////////////////////////////
/// ADDITION IMPLEMENTATIONS ///
////////////////////////////////

/**
 * Adds scalar to every element in array and stores result in ret
 * @tparam T
 * @tparam ArrayType
 * @param array
 * @param scalar
 * @param ret
 * @return
 */
template <typename T, typename ArrayType,
          typename = std::enable_if_t<fetch::math::meta::IsArithmetic<T>>>
meta::IfIsMathArray<ArrayType, void> Add(ArrayType const &array, T const &scalar, ArrayType &ret)
{
  ASSERT(array.shape() == ret.shape());
  auto it1 = array.cbegin();
  auto rit = ret.begin();
  while (it1.is_valid())
  {
    *rit = (*it1) + scalar;
    ++it1;
    ++rit;
  }
}

/**
 * Elementwise array addition for two arrays of the same shape
 * @tparam ArrayType
 * @param array1
 * @param array2
 * @param ret
 * @return
 */
template <typename ArrayType>
meta::IfIsMathArray<ArrayType, void> Add(ArrayType const &array1, ArrayType const &array2,
                                         ArrayType &ret)
{
  ASSERT(array1.shape() == array2.shape());
  ASSERT(array1.shape() == ret.shape());

  if (array1.shape() == array2.shape())
  {
    auto it1 = array1.cbegin();
    auto it2 = array2.cbegin();
    auto rit = ret.begin();
    while (it1.is_valid())
    {
      *rit = (*it1) + (*it2);
      ++it1;
      ++it2;
      ++rit;
    }
  }
}

///////////////////////////////////
/// SUBTRACTION IMPLEMENTATIONS ///
///////////////////////////////////

/**
 * Elementwise subtraction of scalar by an array
 * @tparam ArrayType
 * @tparam T
 * @param scalar
 * @param array
 * @param ret
 * @return
 */
template <typename ArrayType, typename T,
          typename = std::enable_if_t<fetch::math::meta::IsArithmetic<T>>>
meta::IfIsMathArray<ArrayType, void> Subtract(T const &scalar, ArrayType const &array,
                                              ArrayType &ret)
{
  ASSERT(array.size() == ret.size());
  ASSERT(array.shape() == ret.shape());
  auto it1 = array.cbegin();
  auto rit = ret.begin();
  while (it1.is_valid())
  {
    *rit = scalar - *it1;
    ++it1;
    ++rit;
  }
}

/**
 * Elementwise subtraction of an array by a scalar
 * @tparam ArrayType
 * @tparam T
 * @param array
 * @param scalar
 * @param ret
 * @return
 */
template <typename ArrayType, typename T,
          typename = std::enable_if_t<fetch::math::meta::IsArithmetic<T>>>
meta::IfIsMathArray<ArrayType, void> Subtract(ArrayType const &array, T const &scalar,
                                              ArrayType &ret)
{
  ASSERT(array.size() == ret.size());
  auto it1 = array.cbegin();
  auto rit = ret.begin();
  while (it1.is_valid())
  {
    *rit = *it1 - scalar;
    ++it1;
    ++rit;
  }
}

/**
 * Elementwise subtraction for two arrays of equal size
 * @tparam ArrayType
 * @param array1
 * @param array2
 * @param ret
 * @return
 */
template <typename ArrayType>
meta::IfIsMathArray<ArrayType, void> Subtract(ArrayType const &array1, ArrayType const &array2,
                                              ArrayType &ret)
{
  ASSERT(array1.shape() == array2.shape());
  ASSERT(array1.shape() == ret.shape());

  auto it1 = array1.cbegin();
  auto it2 = array2.cbegin();
  auto rit = ret.begin();

  while (it1.is_valid())
  {
    *rit = (*it1) - (*it2);
    ++it1;
    ++it2;
    ++rit;
  }
}

template <typename ArrayType>
::fetch::math::meta::IfIsMathArray<ArrayType, void> Multiply(ArrayType const &obj1,
                                                             ArrayType const &obj2, ArrayType &ret)
{
  ASSERT(obj1.size() == obj2.size());
  ASSERT(ret.size() == obj2.size());
  auto it1 = obj1.begin();
  auto it2 = obj2.begin();
  auto rit = ret.begin();
  while (it1.is_valid())
  {
    *rit = (*it1) * (*it2);
    ++it1;
    ++it2;
    ++rit;
  }
}

template <typename ArrayType, typename T,
          typename = std::enable_if_t<fetch::math::meta::IsArithmetic<T>>>
meta::IfIsMathArray<ArrayType, void> Multiply(ArrayType const &array, T const &scalar,
                                              ArrayType &ret)
{
  ASSERT(array.size() == ret.size());
  auto it1 = array.cbegin();
  auto it2 = ret.begin();
  while (it1.is_valid())
  {
    *it2 = scalar * (*it1);
    ++it1;
    ++it2;
  }
}

/**
 * Divide ony array by another elementwise
 * @tparam ArrayType
 * @param array1
 * @param array2
 * @param ret
 */
template <typename ArrayType>
void Divide(ArrayType const &array1, ArrayType const &array2, ArrayType &ret)
{
  ASSERT(array1.shape() == array2.shape());
  ASSERT(ret.shape() == array2.shape());
  auto it1 = array1.begin();
  auto it2 = array2.begin();
  auto rit = ret.begin();
  while (it1.is_valid())
  {
    *rit = (*it1) / (*it2);
    ++rit;
    ++it1;
    ++it2;
  }
}

/**
 * Divide array elementwise by scalar
 * @tparam ArrayType
 * @tparam T
 * @param array
 * @param scalar
 * @param ret
 * @return
 */
template <typename ArrayType, typename T,
          typename = std::enable_if_t<fetch::math::meta::IsArithmetic<T>>>
meta::IfIsMathArray<ArrayType, void> Divide(ArrayType const &array, T const &scalar, ArrayType &ret)
{
  ASSERT(array.shape() == ret.shape());
  auto it1 = array.cbegin();
  auto rit = ret.begin();
  while (it1.is_valid())
  {
    *rit = (*it1) / scalar;
    ++it1;
    ++rit;
  }
}

/**
 * Divide scalar by array elementwise
 * @tparam ArrayType
 * @tparam T
 * @param scalar
 * @param array
 * @param ret
 * @return
 */
template <typename ArrayType, typename T,
          typename = std::enable_if_t<fetch::math::meta::IsArithmetic<T>>>
meta::IfIsMathArray<ArrayType, void> Divide(T const &scalar, ArrayType const &array, ArrayType &ret)
{
  ASSERT(array.shape() == ret.shape());
  auto it  = array.begin();
  auto it2 = ret.begin();
  while (it.is_valid())
  {
    *it2 = scalar / *it;
    ++it2;
    ++it;
  }
}

}  // namespace implementations

/////////////////
/// ADDITIONS ///
/////////////////

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

template <typename T, typename ArrayType, typename = std::enable_if_t<meta::IsArithmetic<T>>>
meta::IfIsMathArray<ArrayType, ArrayType> Add(ArrayType const &array, T const &scalar)
{
  ArrayType ret{array.shape()};
  implementations::Add(array, scalar, ret);
  return ret;
}
template <typename T, typename ArrayType,
          typename = std::enable_if_t<fetch::math::meta::IsArithmetic<T>>>
meta::IfIsMathArray<ArrayType, ArrayType> Add(T const &scalar, ArrayType const &array)
{
  return Add(array, scalar);
}

template <typename T, typename ArrayType,
          typename = std::enable_if_t<fetch::math::meta::IsArithmetic<T>>>
meta::IfIsMathArray<ArrayType, void> Add(T const &scalar, ArrayType const &array, ArrayType &ret)
{
  ASSERT(array.shape() == ret.shape());
  implementations::Add(array, scalar, ret);
}

template <typename T, typename ArrayType,
          typename = std::enable_if_t<fetch::math::meta::IsArithmetic<T>>>
meta::IfIsMathArray<ArrayType, void> Add(ArrayType const &array, T const &scalar, ArrayType &ret)
{
  ASSERT(array.shape() == ret.shape());
  implementations::Add(array, scalar, ret);
}

template <typename ArrayType>
meta::IfIsMathArray<ArrayType, ArrayType> Add(ArrayType const &array1, ArrayType const &array2)
{
  ASSERT(array1.shape() == array2.shape());
  ArrayType ret{array1.shape()};
  implementations::Add(array1, array2, ret);
  return ret;
}
template <typename ArrayType>
meta::IfIsMathArray<ArrayType, ArrayType> Add(ArrayType const &array1, ArrayType const &array2,
                                              ArrayType &ret)
{
  ASSERT(array1.shape() == array2.shape());
  ASSERT(array1.shape() == ret.shape());
  implementations::Add(array1, array2, ret);
  return ret;
}

///////////////////
/// SUBTRACTION ///
///////////////////

/**
 * Implementation for scalar subtraction. Implementing this helps keeps a uniform interface
 * @tparam T
 * @param scalar1
 * @param scalar2
 * @param ret
 */
template <typename S>
meta::IfIsArithmetic<S, void> Subtract(S const &scalar1, S const &scalar2, S &ret)
{
  ret = scalar1 - scalar2;
}

template <typename S>
meta::IfIsArithmetic<S, S> Subtract(S const &scalar1, S const &scalar2)
{
  S ret;
  Subtract(scalar1, scalar2, ret);
  return ret;
}

template <typename ArrayType, typename T,
          typename = std::enable_if_t<fetch::math::meta::IsArithmetic<T>>>
meta::IfIsMathArray<ArrayType, ArrayType> Subtract(T const &scalar, ArrayType const &array)
{
  ArrayType ret{array.shape()};
  implementations::Subtract(scalar, array, ret);
  return ret;
}
template <typename ArrayType, typename T,
          typename = std::enable_if_t<fetch::math::meta::IsArithmetic<T>>>
meta::IfIsMathArray<ArrayType, ArrayType> Subtract(ArrayType const &array, T const &scalar)
{
  ArrayType ret{array.shape()};
  implementations::Subtract(array, scalar, ret);
  return ret;
}

template <typename ArrayType, typename T,
          typename = std::enable_if_t<fetch::math::meta::IsArithmetic<T>>>
meta::IfIsMathArray<ArrayType, ArrayType> Subtract(T const &scalar, ArrayType const &array,
                                                   ArrayType &ret)
{
  ASSERT(array.shape() == ret.shape());
  implementations::Subtract(scalar, array, ret);
  return ret;
}
template <typename ArrayType, typename T,
          typename = std::enable_if_t<fetch::math::meta::IsArithmetic<T>>>
meta::IfIsMathArray<ArrayType, ArrayType> Subtract(ArrayType const &array, T const &scalar,
                                                   ArrayType &ret)
{
  ASSERT(array.shape() == ret.shape());
  implementations::Subtract(array, scalar, ret);
  return ret;
}

template <typename ArrayType>
meta::IfIsMathArray<ArrayType, ArrayType> Subtract(ArrayType const &obj1, ArrayType const &obj2)
{
  assert(obj1.shape() == obj2.shape());
  ArrayType ret{obj1.shape()};
  implementations::Subtract(obj1, obj2, ret);
  return ret;
}

template <typename ArrayType>
meta::IfIsMathArray<ArrayType, ArrayType> Subtract(ArrayType const &obj1, ArrayType const &obj2,
                                                   ArrayType &ret)
{
  ASSERT(obj1.shape() == obj2.shape());
  ASSERT(obj1.shape() == ret.shape());
  implementations::Subtract(obj1, obj2, ret);
  return ret;
}

////////////////
/// Multiply ///
////////////////

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

template <typename ArrayType, typename T,
          typename = std::enable_if_t<fetch::math::meta::IsArithmetic<T>>>
meta::IfIsMathArray<ArrayType, void> Multiply(T const &scalar, ArrayType const &array,
                                              ArrayType &ret)
{
  implementations::Multiply(array, scalar, ret);
}

template <typename ArrayType, typename T,
          typename = std::enable_if_t<fetch::math::meta::IsArithmetic<T>>>
meta::IfIsMathArray<ArrayType, void> Multiply(ArrayType const &array, T const &scalar,
                                              ArrayType &ret)
{
  Multiply(scalar, array, ret);
}

template <typename ArrayType, typename T,
          typename = std::enable_if_t<fetch::math::meta::IsArithmetic<T>>>
meta::IfIsMathArray<ArrayType, ArrayType> Multiply(T const &scalar, ArrayType const &array)
{
  return Multiply(array, scalar);
}
template <typename ArrayType, typename T,
          typename = std::enable_if_t<fetch::math::meta::IsArithmetic<T>>>
meta::IfIsMathArray<ArrayType, ArrayType> Multiply(ArrayType const &array, T const &scalar)
{
  ArrayType ret{array.shape()};
  Multiply(array, scalar, ret);
  return ret;
}

template <typename ArrayType>
meta::IfIsMathArray<ArrayType, void> Multiply(ArrayType const &obj1, ArrayType const &obj2,
                                              ArrayType &ret)
{
  implementations::Multiply(obj1, obj2, ret);
}

template <typename ArrayType>
meta::IfIsMathArray<ArrayType, ArrayType> Multiply(ArrayType const &obj1, ArrayType const &obj2)
{
  assert(obj1.shape() == obj2.shape());
  ArrayType ret{obj1.shape()};
  Multiply(obj1, obj2, ret);
  return ret;
}
template <typename S>
meta::IfIsArithmetic<S, S> Multiply(S const &scalar1, S const &scalar2)
{
  S ret;
  Multiply(scalar1, scalar2, ret);
  return ret;
}

////////////////
/// DIVISION ///
////////////////

template <typename ArrayType, typename T,
          typename = std::enable_if_t<fetch::math::meta::IsArithmetic<T>>>
meta::IfIsMathArray<ArrayType, ArrayType> Divide(ArrayType const &array, T const &scalar)
{
  ArrayType ret{array.shape()};
  Divide(array, scalar, ret);
  return ret;
}

/**
 * Divide array elementwise by scalar
 * @tparam ArrayType
 * @tparam T
 * @param array
 * @param scalar
 * @param ret
 * @return
 */
template <typename ArrayType, typename T,
          typename = std::enable_if_t<fetch::math::meta::IsArithmetic<T>>>
meta::IfIsMathArray<ArrayType, void> Divide(ArrayType const &array, T const &scalar, ArrayType &ret)
{
  implementations::Divide(array, scalar, ret);
}

/**
 * Divide scalar by every element of array
 * @tparam ArrayType
 * @tparam T
 * @param scalar
 * @param array
 * @param ret
 * @return
 */
template <typename ArrayType, typename T,
          typename = std::enable_if_t<fetch::math::meta::IsArithmetic<T>>>
meta::IfIsMathArray<ArrayType, void> Divide(T const &scalar, ArrayType const &array, ArrayType &ret)
{
  implementations::Divide(scalar, array, ret);
}

/**
 * Fill return array with scalar divided by input array elementwise
 * @tparam T
 * @tparam ArrayType
 * @param scalar
 * @param array
 * @return
 */
template <typename T, typename ArrayType,
          typename = std::enable_if_t<fetch::math::meta::IsArithmetic<T>>>
meta::IfIsMathArray<ArrayType, ArrayType> Divide(T const &scalar, ArrayType const &array)
{
  ArrayType ret{array.shape()};
  Divide(scalar, array, ret);
  return ret;
}

/**
 * Divide one array by another elementwise
 * @tparam ArrayType
 * @param obj1
 * @param obj2
 * @param ret
 * @return
 */
template <typename ArrayType>
meta::IfIsMathArray<ArrayType, void> Divide(ArrayType const &obj1, ArrayType const &obj2,
                                            ArrayType &ret)
{
  assert(obj1.shape() == obj2.shape());
  assert(ret.shape() == obj2.shape());
  implementations::Divide(obj1, obj2, ret);
}

/**
 * Divide arrays elementwise
 * @tparam ArrayType
 * @param obj1
 * @param obj2
 * @return
 */
template <typename ArrayType>
meta::IfIsMathArray<ArrayType, ArrayType> Divide(ArrayType const &obj1, ArrayType const &obj2)
{
  assert(obj1.shape() == obj2.shape());
  ArrayType ret{obj1.shape()};
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