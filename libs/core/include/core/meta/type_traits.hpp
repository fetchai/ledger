#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018 Fetch AI
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

#include <type_traits>

namespace fetch {
namespace meta {

template <bool C, typename R = void>
using EnableIf = typename std::enable_if<C, R>::type;

template <typename T, typename R = T>
using IfIsIntegerLike = EnableIf<(!std::is_same<T, bool>::value) && std::is_integral<T>::value, R>;

template <typename T, typename R = T>
using IfIsFloatLike = EnableIf<std::is_floating_point<T>::value, R>;

template <typename T, typename R = T>
using IfIsBooleanLike = EnableIf<std::is_same<T, bool>::value, R>;

template <typename T, typename R = T>
using IfIsByteArrayLike = EnableIf<std::is_same<T, byte_array::ByteArray>::value, R>;

template <typename T, typename R = T>
using IfIsStdStringLike = EnableIf<std::is_same<T, std::string>::value, R>;

}  // namespace meta
}  // namespace fetch
