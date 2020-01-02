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

#include "meta/tags.hpp"
#include "meta/type_util.hpp"

#include <string>
#include <tuple>
#include <type_traits>

namespace fetch {

namespace byte_array {
class ByteArray;
class ConstByteArray;
}  // namespace byte_array

namespace meta {

template <typename... T>
struct Is
{
  template <typename... Y>
  struct SameAs
  {
    static constexpr bool value = std::is_same<std::tuple<T...>, std::tuple<Y...>>::value;
  };

  template <typename Y0, typename... Y>
  struct SameAsEvery
  {
    static_assert(sizeof...(T) == 1,
                  "There must be just single type T provided in to encapsulating  `Is<T...>` "
                  "template in order to evaluate SameAsEvery<...> sub-template.");
    using FirstT                = typename std::tuple_element<0ull, std::tuple<T...>>::type;
    static constexpr bool value = std::is_same<FirstT, Y0>::value && SameAsEvery<Y...>::value;
  };

  template <typename Y>
  struct SameAsEvery<Y>
  {
    static_assert(sizeof...(T) == 1,
                  "There must be just single type T provided in to encapsulating  `Is<T...>` "
                  "template in order to evaluate SameAsEvery<...> sub-template.");
    using FirstT                = typename std::tuple_element<0ull, std::tuple<T...>>::type;
    static constexpr bool value = std::is_same<FirstT, Y>::value;
  };
};

template <typename T>
static constexpr bool IsBoolean = std::is_same<T, bool>::value;

template <typename T>
static constexpr bool IsUnsignedInteger = std::is_unsigned<T>::value && (!IsBoolean<T>);

template <typename T>
static constexpr bool IsSignedInteger = std::is_signed<T>::value && (!IsBoolean<T>);

template <typename T>
static constexpr bool IsInteger = std::is_integral<T>::value && (!IsBoolean<T>);

template <typename T>
static constexpr bool IsFloat = std::is_floating_point<T>::value;

template <typename T>
static constexpr bool IsConstByteArray = std::is_same<T, fetch::byte_array::ConstByteArray>::value;

template <typename T>
static constexpr bool IsAByteArray =
    type_util::IsAnyOfV<T, byte_array::ByteArray, byte_array::ConstByteArray>;

template <typename T>
static constexpr bool IsStdString = std::is_same<T, std::string>::value;

template <typename T>
static constexpr bool IsStringLike = IsStdString<T> || IsAByteArray<T>;

template <typename T>
static constexpr bool IsNullPtr = std::is_null_pointer<T>::value;

template <class C>
static constexpr bool IsPOD = type_util::SatisfiesAllV<C, std::is_trivial, std::is_standard_layout>;

template <typename T>
constexpr bool IsAny8BitInteger = std::is_same<T, int8_t>::value || std::is_same<T, uint8_t>::value;

template <typename T>
constexpr bool IsNotAny8BitInteger = !IsAny8BitInteger<T>;

template <typename T>
using Decay = std::decay_t<T>;

template <bool C, typename R = void>
using EnableIf = std::enable_if_t<C, R>;

template <typename T, typename U, typename R = void>
using EnableIfSame = EnableIf<std::is_same<T, U>::value, R>;

template <typename T, typename U, typename R = void>
using EnableIfNotSame = EnableIf<!std::is_same<T, U>::value, R>;

template <typename T, typename R = void>
using IfIsInteger = EnableIf<IsInteger<T>, R>;

template <typename T, typename R = void>
using IfIsFloat = EnableIf<IsFloat<T>, R>;

template <typename T, typename R = void>
using IfIsBoolean = EnableIf<IsBoolean<T>, R>;

template <typename T, typename R = void>
using IfIsAByteArray = EnableIf<IsAByteArray<T>, R>;

template <typename T, typename R = void>
using IfIsStdString = EnableIf<IsStdString<T>, R>;

template <typename T, typename R = void>
using IfIsString = EnableIf<IsStringLike<T>, R>;

template <typename T, typename R = void>
using IfIsConstByteArray = EnableIf<IsConstByteArray<T>, R>;

template <typename T, typename R = void>
using IfIsUnsignedInteger = EnableIf<IsUnsignedInteger<T>, R>;

template <typename T, typename R = void>
using IfIsSignedInteger = EnableIf<IsSignedInteger<T>, R>;

//////////////////////////////////////////////////////////////
/// TEMPLATE FOR FUNCTIONS THAT ARE NOT YET IMPLEMENTED
//////////////////////////////////////////////////////////////

template <typename A, typename R>
struct IsNotImplementedImpl
{
};
template <typename A, typename R>
using IfIsNotImplemented = typename IsNotImplementedImpl<A, R>::Type;

template <typename T, typename R = void>
using IfIsNullPtr = EnableIf<IsNullPtr<T>, R>;

template <typename T, typename R = void>
using IfIsPod = EnableIf<IsPOD<T>, R>;

}  // namespace meta
}  // namespace fetch
