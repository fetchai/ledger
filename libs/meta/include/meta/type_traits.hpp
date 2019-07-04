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

#include "meta/tags.hpp"
#include "meta/type_util.hpp"

#include <functional>
#include <string>
#include <type_traits>

namespace fetch {

namespace byte_array {
class ByteArray;
class ConstByteArray;
}  // namespace byte_array

namespace fixed_point {
struct BaseFixedpointType;
}  // namespace fixed_point

namespace meta {

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
static constexpr bool IsFixedPoint = std::is_base_of<fixed_point::BaseFixedpointType, T>::value;

template <typename T>
static constexpr bool IsNotFixedPoint = !IsFixedPoint<T>;

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
using Decay = typename std::decay<T>::type;

template <bool C, typename R = void>
using EnableIf = std::enable_if_t<C, R>;

template <typename T, typename U, typename R = void>
using EnableIfSame = EnableIf<std::is_same<T, U>::value, R>;

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

// template <typename T, std::size_t I, std::size_t F, typename R = void>
// using IfIsFixedPoint = EnableIf<IsFixedPoint<T, I, F>, R>;
template <typename T, typename R = void>
using IfIsFixedPoint = EnableIf<IsFixedPoint<T>, R>;

template <typename T, typename R = void>
using IfIsNotFixedPoint = EnableIf<IsNotFixedPoint<T>, R>;

template <typename T, typename R = void>
using IfIsNullPtr = EnableIf<IsNullPtr<T>, R>;

template <typename T, typename R = void>
using IfIsPod = EnableIf<std::is_pod<T>::value, R>;

template <typename T, typename R = void>
// using IfIsPodOrFixedPoint = EnableIf<std::is_pod<T>::value || IsFixedPoint<T>, R>;
using IfIsPodOrFixedPoint = EnableIf<std::is_pod<T>::value, R>;

template <typename T, typename R = void>
using IfIsArithmetic = EnableIf<std::is_arithmetic<T>::value, R>;

//////////////////////////////////////////////////////////////
/// TEMPLATE FOR FUNCTIONS THAT ARE NOT YET IMPLEMENTED
//////////////////////////////////////////////////////////////

template <typename A, typename R>
struct IsNotImplementedImpl
{
};
template <typename A, typename R>
using IfIsNotImplemented = typename IsNotImplementedImpl<A, R>::Type;

template <class F, class... Args>
using IsNothrowInvocable =
    std::integral_constant<bool, noexcept(std::declval<F>()(std::declval<Args>()...))>;

template <class F, class... Args>
static constexpr auto IsNothrowInvocableV = IsNothrowInvocable<F, Args...>::value;

template <class F, class... Args>
struct InvokeResult
{
  using type = decltype(std::declval<F>()(std::declval<Args>()...));
};

template <class F, class... Args>
using InvokeResultT = typename InvokeResult<F, Args...>::type;

template <class C, class RV, class... Args, class... Args1>
struct InvokeResult<RV (C::*)(Args...), Args1...>
{
  using type = RV;
};

template <class C, class RV, class... Args, class... Args1>
struct InvokeResult<RV (C::*)(Args...) const, Args1...>
{
  using type = RV;
};

namespace detail_ {

template <class Arg>
constexpr decltype((std::declval<std::add_pointer_t<typename std::decay_t<Arg>::type>>(), true))
HasType(Arg &&) noexcept
{
  return true;
}
constexpr bool HasType(...) noexcept
{
  return false;
}

}  // namespace detail_

template <class Arg>
static constexpr bool HasMemberTypeV = detail_::HasType(std::declval<Arg>());

template <class Arg, bool = HasMemberTypeV<Arg>>
struct MemberType
{
  using type = typename Arg::type;
};
template <class Arg, bool b>
using MemberTypeT = typename MemberType<Arg, b>::type;

template <class Arg>
struct MemberType<Arg, false>
{
  // dummy definition for typeless args
  using type = void;
};

template <class L, class R>
struct IsSimilar : std::is_same<L, R>
{
};
template <class L, class R>
static constexpr auto IsSimilarV = IsSimilar<L, R>::value;

template <template <class...> class Ctor, class... LArgs, class... RArgs>
struct IsSimilar<Ctor<LArgs...>, Ctor<RArgs...>> : std::true_type
{
};

}  // namespace meta
}  // namespace fetch
