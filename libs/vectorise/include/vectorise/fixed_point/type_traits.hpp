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

#include "vectorise/fixed_point/fixed_point.hpp"

#include <type_traits>

namespace fetch {
namespace math {
namespace meta {

////////////////////////////
/// FIXED POINT CHECKING ///
////////////////////////////

template <typename DataType>
constexpr bool IsFixedPoint = std::is_base_of<fixed_point::BaseFixedpointType, DataType>::value;

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

template <typename DataType, typename ReturnType>
using IfIsFixedPoint = std::enable_if_t<IsFixedPoint<DataType>, ReturnType>;

template <typename DataType, typename ReturnType>
using IfIsNotFixedPoint = std::enable_if_t<IsNotFixedPoint<DataType>, ReturnType>;

template <typename DataType, typename ReturnType>
using IfIsArithmetic = fetch::meta::EnableIf<IsArithmetic<DataType>, ReturnType>;

template <typename DataType, typename ReturnType>
using IfIsNonFixedPointArithmetic =
    fetch::meta::EnableIf<IsNonFixedPointArithmetic<DataType>, ReturnType>;

template <typename DataType, typename ReturnType>
using IfIsNonFixedPointSignedArithmetic =
    fetch::meta::EnableIf<IsNonFixedPointSignedArithmetic<DataType>, ReturnType>;

template <typename DataType, typename ReturnType>
using IfIsNonFixedPointUnsignedArithmetic =
    fetch::meta::EnableIf<IsNonFixedPointUnsignedArithmetic<DataType>, ReturnType>;

}  // namespace meta
}  // namespace math
}  // namespace fetch
