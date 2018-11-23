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

// TODO (private 179)

#include "core/byte_array/byte_array.hpp"
#include <type_traits>

namespace fetch {
namespace math {

template <typename T, typename C>
class ShapeLessArray;

template <typename T, typename C, bool H, bool W>
class RectangularArray;

namespace linalg {
template <typename T, typename C, typename S>
class Matrix;
}

template <typename T, typename C>
class NDArray;

namespace meta {

////////////////////////////////////
/// ARITHMETIC TYPE CHECKER
////////////////////////////////////

template <bool C, typename R = void>
using EnableIf = typename std::enable_if<C, R>::type;

template <typename T, typename R = T>
using IfIsArithmetic = EnableIf<std::is_arithmetic<T>::value, R>;

////////////////////////////////////
/// MATH LIKE SPECIALIZATIONS
////////////////////////////////////

template <typename A, typename R>
struct IsMathLikeImpl
{
};
template <typename R>
struct IsMathLikeImpl<double, R>
{
  using Type = R;
};
template <typename R>
struct IsMathLikeImpl<float, R>
{
  using Type = R;
};
template <typename R>
struct IsMathLikeImpl<int, R>
{
  using Type = R;
};
template <typename R, typename T, typename C>
struct IsMathLikeImpl<ShapeLessArray<T, C>, R>
{
  using Type = R;
};
template <typename R, typename T, typename C, bool H, bool W>
struct IsMathLikeImpl<fetch::math::RectangularArray<T, C, H, W>, R>
{
  using Type = R;
};
template <typename R, typename T, typename C, typename S>
struct IsMathLikeImpl<fetch::math::linalg::Matrix<T, C, S>, R>
{
  using Type = R;
};
template <typename R, typename T, typename C>
struct IsMathLikeImpl<NDArray<T, C>, R>
{
  using Type = R;
};
template <typename A, typename R>
using IsMathLike = typename IsMathLikeImpl<A, R>::Type;

////////////////////////////////////
/// MATH ARRAY LIKE SPECIALIZATIONS
////////////////////////////////////

template <typename A, typename R>
struct IsMathArrayLikeImpl
{
};
template <typename R, typename T, typename C>
struct IsMathArrayLikeImpl<ShapeLessArray<T, C>, R>
{
  using Type = R;
};
template <typename R, typename T, typename C, bool H, bool W>
struct IsMathArrayLikeImpl<fetch::math::RectangularArray<T, C, H, W>, R>
{
  using Type = R;
};
template <typename R, typename T, typename C, typename S>
struct IsMathArrayLikeImpl<fetch::math::linalg::Matrix<T, C, S>, R>
{
  using Type = R;
};
template <typename R, typename T, typename C>
struct IsMathArrayLikeImpl<NDArray<T, C>, R>
{
  using Type = R;
};
template <typename A, typename R>
using IsMathArrayLike = typename IsMathArrayLikeImpl<A, R>::Type;

////////////////////////////////////
/// MATH ARRAY WITH SHAPE LIKE SPECIALIZATIONS
////////////////////////////////////

template <typename A, typename R>
struct IsMathShapeArrayLikeImpl
{
};
template <typename R, typename T, typename C, bool H, bool W>
struct IsMathShapeArrayLikeImpl<fetch::math::RectangularArray<T, C, H, W>, R>
{
  using Type = R;
};
template <typename R, typename T, typename C, typename S>
struct IsMathShapeArrayLikeImpl<fetch::math::linalg::Matrix<T, C, S>, R>
{
  using Type = R;
};
template <typename R, typename T, typename C>
struct IsMathShapeArrayLikeImpl<NDArray<T, C>, R>
{
  using Type = R;
};
template <typename A, typename R>
using IsMathShapeArrayLike = typename IsMathShapeArrayLikeImpl<A, R>::Type;

////////////////////////////////////
/// MATH ARRAY NO SHAPE LIKE SPECIALIZATIONS
////////////////////////////////////

template <typename A, typename R>
struct IsMathShapelessArrayLikeImpl
{
};
template <typename R, typename T, typename C>
struct IsMathShapelessArrayLikeImpl<ShapeLessArray<T, C>, R>
{
  using Type = R;
};
template <typename A, typename R>
using IsMathShapelessArrayLike = typename IsMathShapelessArrayLikeImpl<A, R>::Type;

////////////////////////////////////
/// BLAS ARRAY SPECIALIZATIONS
////////////////////////////////////

template <typename A, typename R>
struct IsBlasArrayLikeImpl
{
};
template <typename R, typename T, typename C>
struct IsBlasArrayLikeImpl<ShapeLessArray<T, C>, R>
{
  using Type = R;
};
template <typename R, typename T, typename C, bool H, bool W>
struct IsBlasArrayLikeImpl<RectangularArray<T, C, H, W>, R>
{
  using Type = R;
};
template <typename R, typename T, typename C, typename S>
struct IsBlasArrayLikeImpl<linalg::Matrix<T, C, S>, R>
{
  using Type = R;
};
template <typename A, typename R>
using IsBlasArrayLike = typename IsBlasArrayLikeImpl<A, R>::Type;

////////////////////////////////////
/// NON BLAS ARRAY SPECIALIZATIONS
////////////////////////////////////

template <typename A, typename R>
struct IsNonBlasArrayLikeImpl
{
};
template <typename R, typename T, typename C>
struct IsNonBlasArrayLikeImpl<NDArray<T, C>, R>
{
  using Type = R;
};
template <typename A, typename R>
using IsNonBlasArrayLike = typename IsNonBlasArrayLikeImpl<A, R>::Type;

////////////////////////////////////
/// BLAS ARRAY WITH SHAPE SPECIALIZATIONS
////////////////////////////////////

template <typename A, typename R>
struct IsBlasAndShapedArrayLikeImpl
{
};
template <typename R, typename T, typename C, bool H, bool W>
struct IsBlasAndShapedArrayLikeImpl<fetch::math::RectangularArray<T, C, H, W>, R>
{
  using Type = R;
};
template <typename R, typename T, typename C, typename S>
struct IsBlasAndShapedArrayLikeImpl<fetch::math::linalg::Matrix<T, C, S>, R>
{
  using Type = R;
};
template <typename A, typename R>
using IsBlasAndShapedArrayLike = typename IsBlasAndShapedArrayLikeImpl<A, R>::Type;

////////////////////////////////////
/// BLAS ARRAY WITHOUT SHAPE SPECIALIZATIONS
////////////////////////////////////

template <typename A, typename R>
struct IsBlasAndNoShapeArrayLikeImpl
{
};
template <typename R, typename T, typename C>
struct IsBlasAndNoShapeArrayLikeImpl<fetch::math::ShapeLessArray<T, C>, R>
{
  using Type = R;
};
template <typename A, typename R>
using IsBlasAndNoShapeArrayLike = typename IsBlasAndNoShapeArrayLikeImpl<A, R>::Type;

//////////////////////////
/// NOT IMPLEMENTED ERROR
/////////////////////////

template <typename A, typename R>
struct IsNotImplementedImpl
{
};
template <typename A, typename R>
using IsNotImplementedLike = typename IsNotImplementedImpl<A, R>::Type;

}  // namespace meta
}  // namespace math
}  // namespace fetch
