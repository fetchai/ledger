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

#include "core/byte_array/byte_array.hpp"
#include "math/tensor/tensor_declaration.hpp"
#include "meta/type_traits.hpp"
#include "vectorise/fixed_point/type_traits.hpp"
#include "vectorise/meta/math_type_traits.hpp"

#include <type_traits>

namespace fetch {
namespace math {
namespace meta {

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

template <typename DataType, typename ReturnType>
using IfIsUnsignedInteger =
    fetch::meta::EnableIf<fetch::meta::IsUnsignedInteger<DataType> && !IsFloat<DataType>,
                          ReturnType>;

template <typename DataType, typename ReturnType>
using IfIsSignedInteger =
    fetch::meta::EnableIf<fetch::meta::IsSignedInteger<DataType> && !IsFloat<DataType>, ReturnType>;

}  // namespace meta
}  // namespace math
}  // namespace fetch
