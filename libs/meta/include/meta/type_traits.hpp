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

// would be nice to remove these
#include "core/byte_array/byte_array.hpp"
#include "core/byte_array/const_byte_array.hpp"

#include <string>
#include <type_traits>

namespace fetch {
namespace meta {

template <typename T>
constexpr bool IsBoolean = std::is_same<T, bool>::value;

template <typename T>
constexpr bool IsUnsignedInteger = std::is_unsigned<T>::value && (!IsBoolean<T>);

template <typename T>
constexpr bool IsSignedInteger = std::is_signed<T>::value && (!IsBoolean<T>);

template <typename T>
constexpr bool IsInteger = std::is_integral<T>::value && (!IsBoolean<T>);

template <typename T>
constexpr bool IsFloat = std::is_floating_point<T>::value;

template <typename T>
constexpr bool IsConstByteArray = std::is_same<T, fetch::byte_array::ConstByteArray>::value;

template <typename T>
constexpr bool IsAByteArray = (std::is_same<T, byte_array::ByteArray>::value ||
                               std::is_same<T, byte_array::ConstByteArray>::value);

template <typename T>
constexpr bool IsStdString = std::is_same<T, std::string>::value;

template <typename T>
constexpr bool IsStringLike = IsStdString<T> || IsAByteArray<T>;

template <typename T>
constexpr bool IsNullPtr = std::is_null_pointer<T>::value;

template <bool C, typename R = void>
using EnableIf = typename std::enable_if<C, R>::type;

// template <typename T, typename R = T>
// using IfIsArithmetic = EnableIf<std::is_arithmetic<T>::value, R>;

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
using IfIsNullPtr = EnableIf<IsNullPtr<T>, R>;

template <typename T, typename R = void>
using IfIsPod = EnableIf<std::is_pod<T>::value, R>;

template <typename T, typename R = void>
using IfIsArithmetic = EnableIf<std::is_arithmetic<T>::value, R>;

}  // namespace meta
}  // namespace fetch
