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

template <typename T>
constexpr bool IsNonFixedPointArithmetic = std::is_arithmetic<T>::value;

template <typename ArrayType, typename T>
constexpr bool IsArrayScalarType = std::is_same<typename ArrayType::Type, T>::value;

template <typename T, typename R>
using IfIsFixedPoint = EnableIf<IsFixedPoint<T>, R>;

template <typename T, typename R>
using IfIsNotFixedPoint = EnableIf<IsNotFixedPoint<T>, R>;

template <typename ArrayType, typename T, typename R>
using IsSameArrayScalarType = EnableIf<IsArrayScalarType<ArrayType, T>, R>;

template <typename T, typename R>
using IfIsArithmetic = EnableIf<IsArithmetic<T>, R>;

template <typename T, typename R>
using IfIsNonFixedPointArithmetic = EnableIf<IsNonFixedPointArithmetic<T>, R>;

template <typename T, typename R>
using IfIsNotImplemented = fetch::meta::IfIsNotImplemented<T, R>;

template <typename T, typename R>
using IfIsUnsignedInteger = fetch::meta::IfIsUnsignedInteger<T, R>;

template <typename T, typename R>
using IfIsNonFixedSignedArithmetic =
    IfIsNonFixedPointArithmetic<EnableIf<!fetch::meta::IsUnsignedInteger<T>, T>, R>;

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
template <typename R, typename T>
struct IsMathImpl<Tensor<T>, R>
{
  using Type = R;
};
template <typename A, typename R>
using IfIsMath = typename IsMathImpl<A, R>::Type;

//////////////////
/// MATH ARRAY ///
//////////////////

template <typename A, typename R>
struct IsMathArrayImpl
{
};
template <typename R, typename T>
struct IsMathArrayImpl<Tensor<T>, R>
{
  using Type = R;
};

template <typename T, typename R>
using IfIsMathArray = typename IsMathArrayImpl<T, R>::Type;

template <typename T, typename R>
using IfIsMathFixedPointArray = IfIsFixedPoint<typename T::Type, IfIsMathArray<T, R>>;

template <typename T, typename R>
using IfIsMathNonFixedPointArray = IfIsNotFixedPoint<typename T::Type, IfIsMathArray<T, R>>;

//////////////////////////////////////////////////
///// MATH ARRAY & SCALAR TYPE CHECKS TOGETHER ///
//////////////////////////////////////////////////

/**
 * true if ArrayType is a valid math array AND T is a valid arithmetic type
 */
template <typename ArrayType, typename T, typename R = void>
using IfIsArrayScalar = IfIsArithmetic<T, IfIsMathArray<ArrayType, R>>;

/**
 * true if:
 * 1. ArrayType is a valid math array,
 * 2. T is a valid arithmetic type,
 * 3. ArrayType::Type and T are the same type
 */
template <typename ArrayType, typename T, typename R = void>
using IfIsValidArrayScalarPair =
    IsSameArrayScalarType<ArrayType, T, IfIsArrayScalar<ArrayType, T, R>>;

}  // namespace meta
}  // namespace math
}  // namespace fetch
