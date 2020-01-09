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
#include "vectorise/fixed_point/type_traits.hpp"

#include <type_traits>

namespace fetch {

namespace vectorise {
struct BaseVectorRegisterType
{
};
}  // namespace vectorise

namespace math {
namespace meta {

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

template <typename T>
static constexpr bool IsVectorRegister =
    std::is_base_of<vectorise::BaseVectorRegisterType, T>::value;

template <typename T>
static constexpr bool IsNotVectorRegister = !IsVectorRegister<T>;

template <typename T, typename R = void>
using IfIsVectorRegister = fetch::meta::EnableIf<IsVectorRegister<T>, R>;

template <typename T, typename R = void>
using IfIsNotVectorRegister = fetch::meta::EnableIf<IsNotVectorRegister<T>, R>;

////////////////////////////////////////////////
/// TYPES INDIRECTED FROM META / TYPE_TRAITS ///
////////////////////////////////////////////////

template <typename DataType, typename ReturnType>
using IfIsNotImplemented = fetch::meta::IfIsNotImplemented<DataType, ReturnType>;

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
template <typename ReturnType>
struct IsMathImpl<fixed_point::fp32_t, ReturnType>
{
  using Type = ReturnType;
};
template <typename ReturnType>
struct IsMathImpl<fixed_point::fp64_t, ReturnType>
{
  using Type = ReturnType;
};

template <typename DataType, typename ReturnType>
using IfIsMath = typename IsMathImpl<DataType, ReturnType>::Type;

}  // namespace meta
}  // namespace math
}  // namespace fetch
