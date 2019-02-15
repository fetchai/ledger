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

#include "core/byte_array/byte_array.hpp"
#include "core/fixed_point/fixed_point.hpp"

#include "meta/type_traits.hpp"

#include <type_traits>

namespace fetch {

// namespace fixed_point {
//    template <std::size_t I, std::size_t F>
//    class FixedPoint;
//} // fixed_point

namespace math {

template <typename T, typename C>
class ShapelessArray;

template <typename T, typename C, bool H, bool W>
class RectangularArray;

template <typename T, typename C>
class NDArray;

template <typename T>
class Tensor;

namespace meta {

template <bool C, typename R = void>
using EnableIf = typename std::enable_if<C, R>::type;

////////////////////////////
/// FIXED POINT CHECKING ///
////////////////////////////

template <typename T, typename = int>
struct HasFixedPointTag : std::false_type
{
};

// TODO (private 490)
template <typename T>
struct HasFixedPointTag<T, decltype((void)T::fixed_point_tag, 0)> : std::true_type
{
};

template <typename T>
constexpr bool IsFixedPoint = HasFixedPointTag<T>::value;

template <typename T>
constexpr bool IsNotFixedPoint = !IsFixedPoint<T>;

template <typename T>
constexpr bool IsArithmetic = std::is_arithmetic<T>::value || IsFixedPoint<T>;

template <typename T, typename R>
using IfIsFixedPoint = typename std::enable_if<IsFixedPoint<T>, R>::type;

template <typename T, typename R>
using IfIsNotFixedPoint = typename std::enable_if<IsNotFixedPoint<T>, R>::type;

////////////////////////////////////////////////
/// TYPES INDIRECTED FROM META / TYPE_TRAITS ///
////////////////////////////////////////////////

// template <typename T, typename R>
// using IfIsArithmetic = EnableIf<IsArithmetic<T>, R>;
template <typename T, typename R>
using IfIsArithmetic = fetch::meta::IfIsArithmetic<T, R>;

// template <typename T, typename R>
// using IfIsArithmetic = typename std::enable_if<fetch::meta::IfIsArithmetic<T, R> ||
// IsFixedPoint<T>, R>::type;
////using IfIsMathShapeArray = IfIsNotFixedPoint<typename T::Type, typename IsMathShapeArrayImpl<T,
/// R>::Type>;

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
template <typename R, typename T, typename C>
struct IsMathImpl<NDArray<T, C>, R>
{
  using Type = R;
};
template <typename R, typename T>
struct IsMathImpl<Tensor<T>, R>
{
  using Type = R;
};
template <typename A, typename R>
using IfIsMath = typename IsMathImpl<A, R>::Type;

///////////////////////////////////
/// MATH ARRAY - NO FIXED POINT ///
///////////////////////////////////

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
template <typename R, typename T, typename C>
struct IsMathArrayImpl<NDArray<T, C>, R>
{
  using Type = R;
};
template <typename R, typename T>
struct IsMathArrayImpl<Tensor<T>, R>
{
  using Type = R;
};
// template <typename T, typename R>
// using IfIsMathArray = IfIsNotFixedPoint<typename T::Type, typename IsMathArrayImpl<T, R>::Type>;
template <typename T, typename R>
using IfIsMathArray = typename IsMathArrayImpl<T, R>::Type;

///////////////////////////////////////////
/// MATH ARRAY - SHAPE - NO FIXED POINT ///
///////////////////////////////////////////

template <typename A, typename R>
struct IsMathShapeArrayImpl
{
};
template <typename R, typename T, typename C, bool H, bool W>
struct IsMathShapeArrayImpl<fetch::math::RectangularArray<T, C, H, W>, R>
{
  using Type = R;
};
template <typename R, typename T, typename C>
struct IsMathShapeArrayImpl<NDArray<T, C>, R>
{
  using Type = R;
};
template <typename R, typename T>
struct IsMathShapeArrayImpl<Tensor<T>, R>
{
  using Type = R;
};
template <typename T, typename R = void>
// using IfIsMathShapeArray =
//    IfIsNotFixedPoint<typename T::Type, typename IsMathShapeArrayImpl<T, R>::Type>;
using IfIsMathShapeArray = typename IsMathShapeArrayImpl<T, R>::Type;

//////////////////////////////////////////////
/// MATH ARRAY - NO SHAPE - NO FIXED POINT ///
//////////////////////////////////////////////

template <typename A, typename R>
struct IsMathShapelessArrayImpl
{
};
template <typename R, typename T, typename C>
struct IsMathShapelessArrayImpl<ShapelessArray<T, C>, R>
{
  using Type = R;
};

template <typename T, typename R = void>
using IfIsMathShapelessArray =
    IfIsNotFixedPoint<typename T::Type, typename IsMathShapelessArrayImpl<T, R>::Type>;
// template <typename T, typename R = void>
// using IfIsMathShapelessArray = typename IsMathShapelessArrayImpl<T, R>::Type;

////////////////////////////////
/// MATH ARRAY - FIXED POINT ///
////////////////////////////////

template <typename A, typename R>
struct IsMathFixedPointArrayImpl
{
};
template <typename R, typename T, typename C>
struct IsMathFixedPointArrayImpl<ShapelessArray<T, C>, R>
{
  using Type = R;
};
template <typename R, typename T, typename C, bool H, bool W>
struct IsMathFixedPointArrayImpl<fetch::math::RectangularArray<T, C, H, W>, R>
{
  using Type = R;
};
template <typename R, typename T, typename C>
struct IsMathFixedPointArrayImpl<NDArray<T, C>, R>
{
  using Type = R;
};
template <typename T, typename R>
using IfIsMathFixedPointArray =
    IfIsFixedPoint<typename T::Type, typename IsMathFixedPointArrayImpl<T, R>::Type>;

///////////////////////////////////////////
/// MATH ARRAY - SHAPE - FIXED POINT ///
///////////////////////////////////////////

template <typename A, typename R>
struct IsMathFixedPointShapeArrayImpl
{
};
template <typename R, typename T, typename C, bool H, bool W>
struct IsMathFixedPointShapeArrayImpl<fetch::math::RectangularArray<T, C, H, W>, R>
{
  using Type = R;
};
template <typename R, typename T, typename C>
struct IsMathFixedPointShapeArrayImpl<NDArray<T, C>, R>
{
  using Type = R;
};
template <typename T, typename R>
using IfIsMathFixedPointShapeArray =
    IfIsFixedPoint<typename T::Type, typename IsMathFixedPointShapeArrayImpl<T, R>::Type>;

///////////////////////////////////////////
/// MATH ARRAY - NO SHAPE - FIXED POINT ///
///////////////////////////////////////////

template <typename A, typename R>
struct IsMathFixedPointShapelessArrayImpl
{
};
template <typename R, typename T, typename C>
struct IsMathFixedPointShapelessArrayImpl<ShapelessArray<T, C>, R>
{
  using Type = R;
};

template <typename T, typename R = void>
using IfIsMathFixedPointShapelessArray =
    IfIsFixedPoint<typename T::Type, typename IsMathFixedPointShapelessArrayImpl<T, R>::Type>;

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
