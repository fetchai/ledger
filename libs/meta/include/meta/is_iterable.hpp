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

#include <type_traits>

namespace fetch {
namespace meta {
namespace detail {

template <typename T>
auto IsIterableImplementation(int)
    -> decltype(std::begin(std::declval<T &>()) != std::end(std::declval<T &>()),
                ++std::declval<decltype(std::begin(std::declval<T &>())) &>(),
                *std::begin(std::declval<T &>()), std::true_type{});

template <typename T>
std::false_type IsIterableImplementation(...);
}  // namespace detail

template <typename T>
using IsIterable = std::enable_if<decltype(detail::IsIterableImplementation<T>(0)), T>::type;

}  // namespace meta
}  // namespace fetch