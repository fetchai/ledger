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

#include "core/byte_array/byte_array.hpp"
#include "meta/type_traits.hpp"
#include <type_traits>
#include <meta/type_traits.hpp>

namespace fetch {
namespace math {

template <typename T, typename C>
class ShapelessArray;

template <typename T, typename C, bool H, bool W>
class RectangularArray;

namespace linalg {
template <typename T, typename C, typename S>
class Matrix;
}

template <typename T, typename C>
class NDArray;

namespace meta {

////////////////////////////////////////////
/// TYPES INCLUDED FROM META/math_type_traits ///
////////////////////////////////////////////

template <typename T, typename R>
using IfIsArithmetic = fetch::meta::IfIsArithmetic<T, R>;

template <typename T, typename R>
using IfIsNotImplemented = fetch::meta::IfIsNotImplemented<T, R>;

template <typename T>
using IfIsUnsignedInteger = fetch::meta::IfIsUnsignedInteger<T>;

////////////////////////////////////
/// MATH LIKE SPECIALIZATIONS
////////////////////////////////////

template <typename A, typename R>
struct IsMathImpl
{
};
template <typename R>
struct IsMathImpl<double, R>
{
  using Type = R;
};
template <typename R>
struct IsMathImpl<float, R>
{
  using Type = R;
};
template <typename R>
struct IsMathImpl<int, R>
{
  using Type = R;
};
template <typename R, typename T, typename C>
struct IsMathImpl<ShapelessArray<T, C>, R>
{
  using Type = R;
};
template <typename R, typename T, typename C, bool H, bool W>
struct IsMathImpl<fetch::math::RectangularArray<T, C, H, W>, R>
{
  using Type = R;
};
template <typename R, typename T, typename C, typename S>
struct IsMathImpl<fetch::math::linalg::Matrix<T, C, S>, R>
{
  using Type = R;
};
template <typename R, typename T, typename C>
struct IsMathImpl<NDArray<T, C>, R>
{
  using Type = R;
};
template <typename A, typename R>
using IfIsMath = typename IsMathImpl<A, R>::Type;

////////////////////////////////////
/// MATH ARRAY LIKE SPECIALIZATIONS
////////////////////////////////////

template <typename A, typename R>
struct IsMathArrayImpl
{
};
template <typename R, typename T, typename C>
struct IsMathArrayImpl<ShapelessArray<T, C>, R>
{
  using Type = R;
};
template <typename R, typename T, typename C, bool H, bool W>
struct IsMathArrayImpl<fetch::math::RectangularArray<T, C, H, W>, R>
{
  using Type = R;
};
template <typename R, typename T, typename C, typename S>
struct IsMathArrayImpl<fetch::math::linalg::Matrix<T, C, S>, R>
{
  using Type = R;
};
template <typename R, typename T, typename C>
struct IsMathArrayImpl<NDArray<T, C>, R>
{
  using Type = R;
};
template <typename A, typename R>
using IsMathArray = typename IsMathArrayImpl<A, R>::Type;

////////////////////////////////////
/// MATH ARRAY WITH SHAPE LIKE SPECIALIZATIONS
////////////////////////////////////

template <typename A, typename R>
struct IsMathShapeArrayImpl
{
};
template <typename R, typename T, typename C, bool H, bool W>
struct IsMathShapeArrayImpl<fetch::math::RectangularArray<T, C, H, W>, R>
{
  using Type = R;
};
template <typename R, typename T, typename C, typename S>
struct IsMathShapeArrayImpl<fetch::math::linalg::Matrix<T, C, S>, R>
{
  using Type = R;
};
template <typename R, typename T, typename C>
struct IsMathShapeArrayImpl<NDArray<T, C>, R>
{
  using Type = R;
};
template <typename A, typename R>
using IsMathShapeArray = typename IsMathShapeArrayImpl<A, R>::Type;

////////////////////////////////////
/// MATH ARRAY NO SHAPE LIKE SPECIALIZATIONS
////////////////////////////////////

template <typename A, typename R>
struct IsMathShapelessArrayImpl
{
};
template <typename R, typename T, typename C>
struct IsMathShapelessArrayImpl<ShapelessArray<T, C>, R>
{
  using Type = R;
};
template <typename A, typename R>
using IsMathShapelessArray = typename IsMathShapelessArrayImpl<A, R>::Type;

////////////////////////////////////
/// BLAS ARRAY SPECIALIZATIONS
////////////////////////////////////

template <typename A, typename R>
struct IsBlasArrayImpl
{
};
template <typename R, typename T, typename C>
struct IsBlasArrayImpl<ShapelessArray<T, C>, R>
{
  using Type = R;
};
template <typename R, typename T, typename C, bool H, bool W>
struct IsBlasArrayImpl<RectangularArray<T, C, H, W>, R>
{
  using Type = R;
};
template <typename R, typename T, typename C, typename S>
struct IsBlasArrayImpl<linalg::Matrix<T, C, S>, R>
{
  using Type = R;
};
template <typename A, typename R>
using IsBlasArray = typename IsBlasArrayImpl<A, R>::Type;

////////////////////////////////////
/// NON BLAS ARRAY SPECIALIZATIONS
////////////////////////////////////

template <typename A, typename R>
struct IsNonBlasArrayImpl
{
};
template <typename R, typename T, typename C>
struct IsNonBlasArrayImpl<NDArray<T, C>, R>
{
  using Type = R;
};
template <typename A, typename R>
using IsNonBlasArray = typename IsNonBlasArrayImpl<A, R>::Type;

////////////////////////////////////
/// BLAS ARRAY WITH SHAPE SPECIALIZATIONS
////////////////////////////////////

template <typename A, typename R>
struct IsBlasAndShapedArrayImpl
{
};
template <typename R, typename T, typename C, bool H, bool W>
struct IsBlasAndShapedArrayImpl<fetch::math::RectangularArray<T, C, H, W>, R>
{
  using Type = R;
};
template <typename R, typename T, typename C, typename S>
struct IsBlasAndShapedArrayImpl<fetch::math::linalg::Matrix<T, C, S>, R>
{
  using Type = R;
};
template <typename A, typename R>
using IsBlasAndShapedArray = typename IsBlasAndShapedArrayImpl<A, R>::Type;

////////////////////////////////////
/// BLAS ARRAY WITHOUT SHAPE SPECIALIZATIONS
////////////////////////////////////

template <typename A, typename R>
struct IsBlasAndNoShapeArrayLike
{
};
template <typename R, typename T, typename C>
struct IsBlasAndNoShapeArrayLike<fetch::math::ShapelessArray<T, C>, R>
{
  using Type = R;
};
template <typename A, typename R>
using IsBlasAndNoShapeArray = typename IsBlasAndNoShapeArrayLike<A, R>::Type;

}  // namespace meta
}  // namespace math
}  // namespace fetch
