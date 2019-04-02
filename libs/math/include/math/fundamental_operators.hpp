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

///////////////////////
/// IMPLEMENTATIONS ///
///////////////////////

namespace fundamental_operators_details {

/// Additions ///

/**
 * scalar addition implementation
 * @tparam S
 * @param scalar1
 * @param scalar2
 * @param ret
 * @return
 */
template <typename S>
meta::IfIsArithmetic<S, void> Add(S const &scalar1, S const &scalar2, S &ret)
{
  ret = scalar1 + scalar2;
}

/**
 * Array addition implementation
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
  ASSERT(ret.shape() == array2.shape());
  // TODO(804) - no tensor zip implemented
  typename ArrayType::SizeType idx{0};
  for (auto &val : array1)
  {
    Add(val, array2.At(idx), ret.At(idx));
    ++idx;
  }
}

/**
 * Array-Scalar addition
 * @tparam T
 * @tparam ArrayType
 * @param array
 * @param scalar
 * @param ret
 * @return
 */
template <typename ArrayType, typename T>
meta::IfIsValidArrayScalarPair<ArrayType, T, void> Add(ArrayType const &array, T const &scalar,
                                                       ArrayType &ret)
{
  ASSERT(array.shape() == ret.shape());
  // TODO(804) - no tensor zip implemented
  typename ArrayType::SizeType idx{0};
  for (auto &val : array)
  {
    Add(val, scalar, ret.At(idx));
    ++idx;
  }
}

/// Subtractions ///

/**
 * scalar subtraction implementation
 * @tparam S
 * @param scalar1
 * @param scalar2
 * @param ret
 * @return
 */
template <typename S>
meta::IfIsArithmetic<S, void> Subtract(S const &scalar1, S const &scalar2, S &ret)
{
  ret = scalar1 - scalar2;
}

/**
 * Array subtraction implementation
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
  ASSERT(ret.shape() == array2.shape());
  // TODO(804) - no tensor zip implemented
  typename ArrayType::SizeType idx{0};
  for (auto &val : array1)
  {
    Subtract(val, array2.At(idx), ret.At(idx));
    ++idx;
  }
}

/**
 * Scalar-Array subtraction
 * @tparam T
 * @tparam ArrayType
 * @param array
 * @param scalar
 * @param ret
 * @return
 */
template <typename ArrayType, typename T, typename = std::enable_if_t<meta::IsArithmetic<T>>>
meta::IfIsValidArrayScalarPair<ArrayType, T, void> Subtract(T const &scalar, ArrayType const &array,
                                                            ArrayType &ret)
{
  ASSERT(array.shape() == ret.shape());
  // TODO(804) - no tensor zip implemented
  typename ArrayType::SizeType idx{0};
  for (auto &val : array)
  {
    Subtract(scalar, val, ret.At(idx));
    ++idx;
  }
}

/**
 * Array-Scalar subtraction
 * @tparam T
 * @tparam ArrayType
 * @param array
 * @param scalar
 * @param ret
 * @return
 */
template <typename ArrayType, typename T, typename = std::enable_if_t<meta::IsArithmetic<T>>>
meta::IfIsValidArrayScalarPair<ArrayType, T, void> Subtract(ArrayType const &array, T const &scalar,
                                                            ArrayType &ret)
{
  ASSERT(array.shape() == ret.shape());
  // TODO(804) - no tensor zip implemented
  typename ArrayType::SizeType idx{0};
  for (auto &val : array)
  {
    Subtract(val, scalar, ret.At(idx));
    ++idx;
  }
}

/// Multiply ///

/**
 * scalar multiplication implementation
 * @tparam S
 * @param scalar1
 * @param scalar2
 * @param ret
 * @return
 */
template <typename S>
meta::IfIsArithmetic<S, void> Multiply(S const &scalar1, S const &scalar2, S &ret)
{
  ret = scalar1 * scalar2;
}

/**
 * Array elementwise multiplication implementation
 * @tparam ArrayType
 * @param array1
 * @param array2
 * @param ret
 * @return
 */
template <typename ArrayType>
meta::IfIsMathArray<ArrayType, void> Multiply(ArrayType const &array1, ArrayType const &array2,
                                              ArrayType &ret)
{
  ASSERT(array1.shape() == array2.shape());
  ASSERT(ret.shape() == array2.shape());
  // TODO(804) - no tensor zip implemented
  typename ArrayType::SizeType idx{0};
  for (auto &val : array1)
  {
    Multiply(val, array2.At(idx), ret.At(idx));
    ++idx;
  }
}

/**
 * Array-Scalar multiplication
 * @tparam T
 * @tparam ArrayType
 * @param array
 * @param scalar
 * @param ret
 * @return
 */
template <typename ArrayType, typename T>
meta::IfIsValidArrayScalarPair<ArrayType, T, void> Multiply(ArrayType const &array, T const &scalar,
                                                            ArrayType &ret)
{
  ASSERT(array.shape() == ret.shape());
  // TODO(804) - no tensor zip implemented
  typename ArrayType::SizeType idx{0};
  for (auto &val : array)
  {
    Multiply(val, scalar, ret.At(idx));
    ++idx;
  }
}

/// Divide ///

/**
 * scalar division implementation
 * @tparam S
 * @param scalar1
 * @param scalar2
 * @param ret
 * @return
 */
template <typename S>
meta::IfIsArithmetic<S, void> Divide(S const &scalar1, S const &scalar2, S &ret)
{
  ASSERT(scalar2 != S(0));
  ret = scalar1 / scalar2;
}

/**
 * Array elementwise division implementation
 * @tparam ArrayType
 * @param array1
 * @param array2
 * @param ret
 * @return
 */
template <typename ArrayType>
meta::IfIsMathArray<ArrayType, void> Divide(ArrayType const &array1, ArrayType const &array2,
                                            ArrayType &ret)
{
  ASSERT(array1.shape() == array2.shape());
  ASSERT(ret.shape() == array2.shape());
  // TODO(804) - no tensor zip implemented
  typename ArrayType::SizeType idx{0};
  for (auto &val : array1)
  {
    Divide(val, array2.At(idx), ret.At(idx));
    ++idx;
  }
}

/**
 * Scalar-Array division
 * @tparam T
 * @tparam ArrayType
 * @param array
 * @param scalar
 * @param ret
 * @return
 */
template <typename ArrayType, typename T, typename = std::enable_if_t<meta::IsArithmetic<T>>>
meta::IfIsValidArrayScalarPair<ArrayType, T, void> Divide(T const &scalar, ArrayType const &array,
                                                          ArrayType &ret)
{
  ASSERT(array.shape() == ret.shape());
  // TODO(804) - no tensor zip implemented
  typename ArrayType::SizeType idx{0};
  for (auto &val : array)
  {
    Divide(scalar, val, ret.At(idx));
    ++idx;
  }
}

/**
 * Array-Scalar division
 * @tparam T
 * @tparam ArrayType
 * @param array
 * @param scalar
 * @param ret
 * @return
 */
template <typename ArrayType, typename T, typename = std::enable_if_t<meta::IsArithmetic<T>>>
meta::IfIsValidArrayScalarPair<ArrayType, T, void> Divide(ArrayType const &array, T const &scalar,
                                                          ArrayType &ret)
{
  ASSERT(array.shape() == ret.shape());
  // TODO(804) - no tensor zip implemented
  typename ArrayType::SizeType idx{0};
  for (auto &val : array)
  {
    Divide(val, scalar, ret.At(idx));
    ++idx;
  }
}

}  // namespace fundamental_operators_details

//////////////////
/// INTERFACES ///
//////////////////

/// ADDITIONS

/// SCALAR - SCALAR
template <typename S>
meta::IfIsArithmetic<S, void> Add(S const &scalar1, S const &scalar2, S &ret)
{
  fundamental_operators_details::Add(scalar1, scalar2, ret);
}

template <typename S>
meta::IfIsArithmetic<S, S> Add(S const &scalar1, S const &scalar2)
{
  S ret;
  fundamental_operators_details::Add(scalar1, scalar2, ret);
  return ret;
}

/// ARRAY - SCALAR
template <typename ArrayType, typename T, typename = std::enable_if_t<meta::IsArithmetic<T>>>
meta::IfIsValidArrayScalarPair<ArrayType, T, void> Add(ArrayType const &array, T const &scalar,
                                                       ArrayType &ret)
{
  ASSERT(ret.shape() == array.shape());
  fundamental_operators_details::Add(array, scalar, ret);
}
template <typename ArrayType, typename T, typename = std::enable_if_t<meta::IsArithmetic<T>>>
meta::IfIsValidArrayScalarPair<ArrayType, T, ArrayType> Add(ArrayType const &array, T const &scalar)
{
  ArrayType ret{array.shape()};
  fundamental_operators_details::Add(array, scalar, ret);
  return ret;
}
template <typename ArrayType, typename T, typename = std::enable_if_t<meta::IsArithmetic<T>>>
meta::IfIsValidArrayScalarPair<ArrayType, T, void> Add(T const &scalar, ArrayType const &array,
                                                       ArrayType &ret)
{
  ASSERT(ret.shape() == array.shape());
  fundamental_operators_details::Add(array, scalar, ret);
}
template <typename ArrayType, typename T, typename = std::enable_if_t<meta::IsArithmetic<T>>>
meta::IfIsValidArrayScalarPair<ArrayType, T, ArrayType> Add(T const &scalar, ArrayType const &array)
{
  ArrayType ret{array.shape()};
  fundamental_operators_details::Add(array, scalar, ret);
  return ret;
}

/// ARRAY - ARRAY
template <typename ArrayType>
meta::IfIsMathArray<ArrayType, void> Add(ArrayType const &array1, ArrayType const &array2,
                                         ArrayType &ret)
{
  ASSERT(array1.shape() == array2.shape());
  ASSERT(array1.shape() == ret.shape());
  fundamental_operators_details::Add(array1, array2, ret);
}
template <typename ArrayType>
meta::IfIsMathArray<ArrayType, ArrayType> Add(ArrayType const &array1, ArrayType const &array2)
{
  ASSERT(array1.shape() == array2.shape());
  ArrayType ret{array1.shape()};
  fundamental_operators_details::Add(array1, array2, ret);
  return ret;
}

/// Subtractions

/// SCALAR - SCALAR
template <typename S>
meta::IfIsArithmetic<S, void> Subtract(S const &scalar1, S const &scalar2, S &ret)
{
  fundamental_operators_details::Subtract(scalar1, scalar2, ret);
}

template <typename S>
meta::IfIsArithmetic<S, S> Subtract(S const &scalar1, S const &scalar2)
{
  S ret;
  fundamental_operators_details::Subtract(scalar1, scalar2, ret);
  return ret;
}

/// ARRAY - SCALAR
template <typename ArrayType, typename T, typename = std::enable_if_t<meta::IsArithmetic<T>>>
meta::IfIsValidArrayScalarPair<ArrayType, T, void> Subtract(ArrayType const &array, T const &scalar,
                                                            ArrayType &ret)
{
  ASSERT(ret.shape() == array.shape());
  fundamental_operators_details::Subtract(array, scalar, ret);
}
template <typename ArrayType, typename T, typename = std::enable_if_t<meta::IsArithmetic<T>>>
meta::IfIsValidArrayScalarPair<ArrayType, T, ArrayType> Subtract(ArrayType const &array,
                                                                 T const &        scalar)
{
  ArrayType ret{array.shape()};
  fundamental_operators_details::Subtract(array, scalar, ret);
  return ret;
}
template <typename ArrayType, typename T, typename = std::enable_if_t<meta::IsArithmetic<T>>>
meta::IfIsValidArrayScalarPair<ArrayType, T, void> Subtract(T const &scalar, ArrayType const &array,
                                                            ArrayType &ret)
{
  ASSERT(ret.shape() == array.shape());
  fundamental_operators_details::Subtract(scalar, array, ret);
}
template <typename ArrayType, typename T, typename = std::enable_if_t<meta::IsArithmetic<T>>>
meta::IfIsValidArrayScalarPair<ArrayType, T, ArrayType> Subtract(T const &        scalar,
                                                                 ArrayType const &array)
{
  ArrayType ret{array.shape()};
  fundamental_operators_details::Subtract(scalar, array, ret);
  return ret;
}

/// ARRAY - ARRAY
template <typename ArrayType>
meta::IfIsMathArray<ArrayType, void> Subtract(ArrayType const &array1, ArrayType const &array2,
                                              ArrayType &ret)
{
  ASSERT(array1.shape() == array2.shape());
  ASSERT(array1.shape() == ret.shape());
  fundamental_operators_details::Subtract(array1, array2, ret);
}
template <typename ArrayType>
meta::IfIsMathArray<ArrayType, ArrayType> Subtract(ArrayType const &array1, ArrayType const &array2)
{
  ASSERT(array1.shape() == array2.shape());
  ArrayType ret{array1.shape()};
  fundamental_operators_details::Subtract(array1, array2, ret);
  return ret;
}

/// Multiplications

/// SCALAR - SCALAR
template <typename S>
meta::IfIsArithmetic<S, void> Multiply(S const &scalar1, S const &scalar2, S &ret)
{
  fundamental_operators_details::Multiply(scalar1, scalar2, ret);
}

template <typename S>
meta::IfIsArithmetic<S, S> Multiply(S const &scalar1, S const &scalar2)
{
  S ret;
  fundamental_operators_details::Multiply(scalar1, scalar2, ret);
  return ret;
}

/// ARRAY - SCALAR
template <typename ArrayType, typename T, typename = std::enable_if_t<meta::IsArithmetic<T>>>
meta::IfIsValidArrayScalarPair<ArrayType, T, void> Multiply(ArrayType const &array, T const &scalar,
                                                            ArrayType &ret)
{
  ASSERT(ret.shape() == array.shape());
  fundamental_operators_details::Multiply(array, scalar, ret);
}
template <typename ArrayType, typename T, typename = std::enable_if_t<meta::IsArithmetic<T>>>
meta::IfIsValidArrayScalarPair<ArrayType, T, ArrayType> Multiply(ArrayType const &array,
                                                                 T const &        scalar)
{
  ArrayType ret{array.shape()};
  fundamental_operators_details::Multiply(array, scalar, ret);
  return ret;
}
template <typename ArrayType, typename T, typename = std::enable_if_t<meta::IsArithmetic<T>>>
meta::IfIsValidArrayScalarPair<ArrayType, T, void> Multiply(T const &scalar, ArrayType const &array,
                                                            ArrayType &ret)
{
  ASSERT(ret.shape() == array.shape());
  fundamental_operators_details::Multiply(array, scalar, ret);
}
template <typename ArrayType, typename T, typename = std::enable_if_t<meta::IsArithmetic<T>>>
meta::IfIsValidArrayScalarPair<ArrayType, T, ArrayType> Multiply(T const &        scalar,
                                                                 ArrayType const &array)
{
  ArrayType ret{array.shape()};
  fundamental_operators_details::Multiply(array, scalar, ret);
  return ret;
}

/// ARRAY - ARRAY
template <typename ArrayType>
meta::IfIsMathArray<ArrayType, void> Multiply(ArrayType const &array1, ArrayType const &array2,
                                              ArrayType &ret)
{
  ASSERT(array1.shape() == array2.shape());
  ASSERT(array1.shape() == ret.shape());
  fundamental_operators_details::Multiply(array1, array2, ret);
}
template <typename ArrayType>
meta::IfIsMathArray<ArrayType, ArrayType> Multiply(ArrayType const &array1, ArrayType const &array2)
{
  ASSERT(array1.shape() == array2.shape());
  ArrayType ret{array1.shape()};
  fundamental_operators_details::Multiply(array1, array2, ret);
  return ret;
}

/// Division

/// SCALAR - SCALAR
template <typename S>
meta::IfIsArithmetic<S, void> Divide(S const &scalar1, S const &scalar2, S &ret)
{
  fundamental_operators_details::Divide(scalar1, scalar2, ret);
}

template <typename S>
meta::IfIsArithmetic<S, S> Divide(S const &scalar1, S const &scalar2)
{
  S ret;
  fundamental_operators_details::Divide(scalar1, scalar2, ret);
  return ret;
}

/// ARRAY - SCALAR
template <typename ArrayType, typename T, typename = std::enable_if_t<meta::IsArithmetic<T>>>
meta::IfIsValidArrayScalarPair<ArrayType, T, void> Divide(ArrayType const &array, T const &scalar,
                                                          ArrayType &ret)
{
  ASSERT(ret.shape() == array.shape());
  fundamental_operators_details::Divide(array, scalar, ret);
}
template <typename ArrayType, typename T, typename = std::enable_if_t<meta::IsArithmetic<T>>>
meta::IfIsValidArrayScalarPair<ArrayType, T, ArrayType> Divide(ArrayType const &array,
                                                               T const &        scalar)
{
  ArrayType ret{array.shape()};
  fundamental_operators_details::Divide(array, scalar, ret);
  return ret;
}
template <typename ArrayType, typename T, typename = std::enable_if_t<meta::IsArithmetic<T>>>
meta::IfIsValidArrayScalarPair<ArrayType, T, void> Divide(T const &scalar, ArrayType const &array,
                                                          ArrayType &ret)
{
  ASSERT(ret.shape() == array.shape());
  fundamental_operators_details::Divide(scalar, array, ret);
}
template <typename ArrayType, typename T, typename = std::enable_if_t<meta::IsArithmetic<T>>>
meta::IfIsValidArrayScalarPair<ArrayType, T, ArrayType> Divide(T const &        scalar,
                                                               ArrayType const &array)
{
  ArrayType ret{array.shape()};
  fundamental_operators_details::Divide(scalar, array, ret);
  return ret;
}

/// ARRAY - ARRAY
template <typename ArrayType>
meta::IfIsMathArray<ArrayType, void> Divide(ArrayType const &array1, ArrayType const &array2,
                                            ArrayType &ret)
{
  ASSERT(array1.shape() == array2.shape());
  ASSERT(array1.shape() == ret.shape());
  fundamental_operators_details::Divide(array1, array2, ret);
}
template <typename ArrayType>
meta::IfIsMathArray<ArrayType, ArrayType> Divide(ArrayType const &array1, ArrayType const &array2)
{
  ASSERT(array1.shape() == array2.shape());
  ArrayType ret{array1.shape()};
  fundamental_operators_details::Divide(array1, array2, ret);
  return ret;
}

/////////////////
/// OPERATORS ///
/////////////////

template <typename OtherType>
meta::IfIsMath<OtherType, OtherType> operator+(OtherType &left, OtherType const &right)
{
  OtherType ret;
  fundamental_operators_details::Add(left, right, ret);
  return ret;
}

template <typename OtherType>
meta::IfIsMath<OtherType, void> operator+=(OtherType &left, OtherType const &right)
{
  fundamental_operators_details::Add(left, right, left);
}

template <typename OtherType>
meta::IfIsMath<OtherType, OtherType> operator-(OtherType &left, OtherType const &right)
{
  OtherType ret;
  fundamental_operators_details::Subtract(left, right, ret);
  return ret;
}

template <typename OtherType>
meta::IfIsMath<OtherType, void> operator-=(OtherType &left, OtherType const &right)
{
  fundamental_operators_details::Subtract(left, right, left);
}

template <typename OtherType>
meta::IfIsMath<OtherType, OtherType> operator*(OtherType &left, OtherType const &right)
{
  OtherType ret;
  fundamental_operators_details::Multiply(left, right, ret);
  return ret;
}

template <typename OtherType>
meta::IfIsMath<OtherType, void> operator*=(OtherType &left, OtherType const &right)
{
  fundamental_operators_details::Multiply(left, right, left);
}

template <typename OtherType>
meta::IfIsMath<OtherType, OtherType> operator/(OtherType &left, OtherType const &right)
{
  OtherType ret;
  fundamental_operators_details::Divide(left, right, ret);
  return ret;
}

template <typename OtherType>
meta::IfIsMath<OtherType, void> operator/=(OtherType &left, OtherType const &right)
{
  fundamental_operators_details::Divide(left, right, left);
}

}  // namespace math
}  // namespace fetch
