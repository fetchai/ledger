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
namespace math {

template <typename T, typename C>
class ShapelessArray;

template <typename T, typename C>
class Tensor;

namespace meta {

template <bool C, typename R = void>
using EnableIf = typename std::enable_if<C, R>::type;

////////////////////////////
/// FIXED POINT CHECKING ///
////////////////////////////

template <typename T>
constexpr bool IsFixedPoint =  std::is_base_of<fixed_point::BaseFixedpointType, T  >::value;

template <typename T>
constexpr bool IsNotFixedPoint = !IsFixedPoint<T>;

template <typename T>
constexpr bool IsArithmetic = std::is_arithmetic<T>::value || IsFixedPoint<T>;

template <typename T>
constexpr bool IsNonFixedPointArithmetic = std::is_arithmetic<T>::value;

template <typename T, typename R>
using IfIsFixedPoint = typename std::enable_if<IsFixedPoint<T>, R>::type;

template <typename T, typename R>
using IfIsNotFixedPoint = typename std::enable_if<IsNotFixedPoint<T>, R>::type;

////////////////////////////////////////////////
/// TYPES INDIRECTED FROM META / TYPE_TRAITS ///
////////////////////////////////////////////////

template <typename T, typename R>
using IfIsArithmetic = EnableIf<IsArithmetic<T>, R>;

template <typename T, typename R>
using IfIsNonFixedPointArithmetic = EnableIf<IsNonFixedPointArithmetic<T>, R>;

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

template <typename R, typename T, typename C>
struct IsMathImpl<Tensor<T, C>, R>
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

template <typename R, typename T, typename C>
struct IsMathArrayImpl<Tensor<T, C>, R>
{
  using Type = R;
};
template <typename T, typename R>
using IfIsMathArray = typename IsMathArrayImpl<T, R>::Type;

template <typename T, typename R>
using IfIsMathNonFixedPointArray = IfIsNotFixedPoint<typename T::Type, IfIsMathArray<T, R>>;

///////////////////////////////////////////
/// MATH ARRAY - SHAPE - NO FIXED POINT ///
///////////////////////////////////////////

template <typename A, typename R>
struct IsMathShapeArrayImpl
{
};

template <typename R, typename T, typename C>
struct IsMathShapeArrayImpl<Tensor<T, C>, R>
{
  using Type = R;
};
template <typename T, typename R = void>
using IfIsMathShapeArray = typename IsMathShapeArrayImpl<T, R>::Type;

template <typename T, typename R = void>
using IfIsMathNonFixedPointShapeArray =
    IfIsNotFixedPoint<typename T::Type, IfIsMathShapeArray<T, R>>;

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
using IfIsMathShapelessArray = typename IsMathShapelessArrayImpl<T, R>::Type;

template <typename T, typename R = void>
using IfIsMathNonFixedPointShapelessArray =
    IfIsNotFixedPoint<typename T::Type, IfIsMathShapelessArray<T, R>>;

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

template <typename R, typename T, typename C>
struct IsMathFixedPointArrayImpl<fetch::math::Tensor<T, C>, R>
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

template <typename R, typename T, typename C>
struct IsMathFixedPointShapeArrayImpl<fetch::math::Tensor<T, C>, R>
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

//////////////////////////////////////
///// BLAS ARRAY SPECIALIZATIONS
//////////////////////////////////////

template <typename A, typename R>
struct IsBlasArrayImpl
{
};
template <typename R, typename T, typename C>
struct IsBlasArrayImpl<ShapelessArray<T, C>, R>
{
  using Type = R;
};

template <typename T, typename R = void>
using IfIsBlasArray = IfIsNotFixedPoint<typename T::Type, typename IsBlasArrayImpl<T, R>::Type>;

//////////////////////////////////////
///// NON BLAS ARRAY SPECIALIZATIONS
//////////////////////////////////////

template <typename A, typename R>
struct IsNonBlasArrayImpl
{
};
template <typename R, typename T, typename C>
struct IsNonBlasArrayImpl<Tensor<T, C>, R>
{
  using Type = R;
};
template <typename A, typename R>
using IfIsNonBlasArray =
    IfIsNotFixedPoint<typename A::Type, typename IsNonBlasArrayImpl<A, R>::Type>;


}  // namespace meta
}  // namespace math
}  // namespace fetch
