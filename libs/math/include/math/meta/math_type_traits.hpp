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
#include "vectorise/fixed_point/type_traits.hpp"

#include <type_traits>

namespace fetch {
namespace math {

namespace meta {

template <bool C, typename ReturnType = void>
using EnableIf = std::enable_if_t<C, ReturnType>;

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
using IfVectorSupportFor = std::enable_if_t<HasVectorSupport<T>::value, R>;
template <typename T, typename R>
using IfNoVectorSupportFor = std::enable_if_t<!HasVectorSupport<T>::value, R>;

////////////////////////////////////////////////
/// TYPES INDIRECTED FROM META / TYPE_TRAITS ///
////////////////////////////////////////////////

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
