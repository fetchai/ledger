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

#include "math/tensor_declaration.hpp"
#include "meta/type_traits.hpp"
#include "vectorise/fixed_point/fixed_point.hpp"

#include <type_traits>

namespace fetch {
namespace math {

namespace meta {

template <bool C, typename ReturnType = void>
using EnableIf = typename std::enable_if<C, ReturnType>::type;

////////////////////////////////////
/// REGISTER OF VECTORISED TYPES ///
////////////////////////////////////

template <typename T>
struct HasVectorSupport
{
  enum
  {
    value = 0
  };
};

template <>
struct HasVectorSupport<double>
{
  enum
  {
    value = 1
  };
};

template <>
struct HasVectorSupport<float>
{
  enum
  {
    value = 1
  };
};

template <typename T, typename R>
using IfVectorSupportFor = typename std::enable_if<HasVectorSupport<T>::value, R>::type;
template <typename T, typename R>
using IfNoVectorSupportFor = typename std::enable_if<!HasVectorSupport<T>::value, R>::type;

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

template <typename DataType, typename ReturnType>
using IfIsFixedPoint = typename std::enable_if<IsFixedPoint<DataType>, ReturnType>::type;

template <typename DataType, typename ReturnType>
using IfIsNotFixedPoint = typename std::enable_if<IsNotFixedPoint<DataType>, ReturnType>::type;

////////////////////////////////////////////////
/// TYPES INDIRECTED FROM META / TYPE_TRAITS ///
////////////////////////////////////////////////

template <typename DataType, typename ReturnType>
using IfIsArithmetic = EnableIf<IsArithmetic<DataType>, ReturnType>;

template <typename DataType, typename ReturnType>
using IfIsNonFixedPointArithmetic = EnableIf<IsNonFixedPointArithmetic<DataType>, ReturnType>;

template <typename DataType, typename ReturnType>
using IfIsNotImplemented = fetch::meta::IfIsNotImplemented<DataType, ReturnType>;

template <typename DataType>
using IfIsUnsignedInteger = fetch::meta::IfIsUnsignedInteger<DataType>;

////////////////////////////////////
/// MATH LIKE SPECIALIZATIONS
////////////////////////////////////

template <typename A, typename ReturnType>
struct IsMathImpl
{
};
template <typename ReturnType>
struct IsMathImpl<double, ReturnType>
{
  using Type = ReturnType;
};
template <typename ReturnType>
struct IsMathImpl<float, ReturnType>
{
  using Type = ReturnType;
};
template <typename ReturnType>
struct IsMathImpl<int, ReturnType>
{
  using Type = ReturnType;
};

template <typename DataType, typename ContainerType /*template<class> class ContainerType*/,
          typename ReturnType>
struct IsMathImpl<Tensor<DataType, ContainerType>, ReturnType>
{
  using Type = ReturnType;
};

template <typename DataType, typename ReturnType>
using IfIsMath = typename IsMathImpl<DataType, ReturnType>::Type;

//////////////////
/// MATH ARRAY ///
//////////////////

template <typename DataType, typename ReturnType>
struct IsMathArrayImpl
{
};
template <typename DataType, typename ContainerType /*template<class> class ContainerType*/,
          typename ReturnType>
struct IsMathArrayImpl<Tensor<DataType, ContainerType>, ReturnType>
{
  using Type = ReturnType;
};
template <typename DataType, typename ReturnType>
using IfIsMathArray = typename IsMathArrayImpl<DataType, ReturnType>::Type;

template <typename DataType, typename ReturnType>
using IfIsMathFixedPointArray =
    IfIsFixedPoint<typename DataType::Type, IfIsMathArray<DataType, ReturnType>>;

template <typename DataType, typename ReturnType>
using IfIsMathNonFixedPointArray =
    IfIsNotFixedPoint<typename DataType::Type, IfIsMathArray<DataType, ReturnType>>;

}  // namespace meta
}  // namespace math
}  // namespace fetch
