#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018-2020 Fetch.AI Limited
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

#include "meta/type_traits.hpp"

#include <type_traits>

namespace fetch {

namespace fixed_point {
struct BaseFixedpointType;
template <uint16_t I, uint16_t F>
class FixedPoint;
using fp32_t  = FixedPoint<16, 16>;
using fp64_t  = FixedPoint<32, 32>;
using fp128_t = FixedPoint<64, 64>;
}  // namespace fixed_point
namespace math {
namespace meta {

////////////////////////////
/// FIXED POINT CHECKING ///
////////////////////////////

template <typename DataType>
constexpr bool IsFixedPoint = std::is_base_of<fixed_point::BaseFixedpointType, DataType>::value;

template <typename DataType>
constexpr bool IsInteger = fetch::meta::IsInteger<DataType>;

template <typename DataType>
constexpr bool IsFloat = fetch::meta::IsFloat<DataType>;

template <typename DataType>
constexpr bool IsNotFixedPoint = !IsFixedPoint<DataType>;

template <typename DataType>
constexpr bool IsArithmetic = std::is_arithmetic<DataType>::value || IsFixedPoint<DataType>;

template <typename DataType>
constexpr bool IsIntegerOrFixedPoint = fetch::meta::IsInteger<DataType> || IsFixedPoint<DataType>;

template <typename DataType>
constexpr bool IsNonFixedPointArithmetic = std::is_arithmetic<DataType>::value;

template <typename DataType>
constexpr bool                            IsNonFixedPointSignedArithmetic =
    std::is_arithmetic<DataType>::value &&std::is_signed<DataType>::value;

template <typename DataType>
constexpr bool IsNonFixedPointUnsignedArithmetic =
    std::is_arithmetic<DataType>::value && !(std::is_signed<DataType>::value);

template <typename DataType, typename ReturnType = void>
using IfIsFloat = fetch::meta::EnableIf<IsFloat<DataType>, ReturnType>;

template <typename DataType, typename ReturnType = void>
using IfIsInteger = fetch::meta::EnableIf<IsInteger<DataType>, ReturnType>;

template <typename DataType, typename ReturnType = void>
using IfIsFixedPoint = fetch::meta::EnableIf<IsFixedPoint<DataType>, ReturnType>;

template <typename DataType, typename ReturnType = void>
using IfIsNotFixedPoint = fetch::meta::EnableIf<IsNotFixedPoint<DataType>, ReturnType>;

template <typename DataType, typename ReturnType = void>
using IfIsArithmetic = fetch::meta::EnableIf<IsArithmetic<DataType>, ReturnType>;

template <typename DataType, typename ReturnType = void>
using IfIsNonFixedPointArithmetic =
    fetch::meta::EnableIf<IsNonFixedPointArithmetic<DataType>, ReturnType>;

template <typename DataType, typename ReturnType = void>
using IfIsNonFixedPointSignedArithmetic =
    fetch::meta::EnableIf<IsNonFixedPointSignedArithmetic<DataType>, ReturnType>;

template <typename DataType, typename ReturnType = void>
using IfIsNonFixedPointUnsignedArithmetic =
    fetch::meta::EnableIf<IsNonFixedPointUnsignedArithmetic<DataType>, ReturnType>;

template <typename T>
constexpr bool IsPodOrFixedPoint = fetch::meta::IsPOD<T> || IsFixedPoint<T>;

template <typename T, typename R = void>
using IfIsPodOrFixedPoint = fetch::meta::EnableIf<IsPodOrFixedPoint<T>, R>;

template <typename T>
constexpr bool IsFixedPoint128 = std::is_same<T, fixed_point::FixedPoint<64, 64>>::value;

template <typename T>
constexpr bool IsNotFixedPoint128 = !IsFixedPoint128<T> && IsFixedPoint<T>;

template <typename DataType, typename ReturnType = void>
using IfIsFixedPoint128 = fetch::meta::EnableIf<IsFixedPoint128<DataType>, ReturnType>;

template <typename DataType, typename ReturnType = void>
using IfIsNotFixedPoint128 = fetch::meta::EnableIf<IsNotFixedPoint128<DataType>, ReturnType>;

}  // namespace meta
}  // namespace math
}  // namespace fetch
