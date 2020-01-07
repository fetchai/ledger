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

#include <iterator>
#include <type_traits>
#include <utility>

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

template <typename T, typename R>
using IsIterable = std::enable_if_t<
    std::is_same<decltype(detail::IsIterableImplementation<T>(0)), std::true_type>::value, R>;

template <typename T1, typename T2, typename R>
using IsIterableTwoArg = std::enable_if_t<
    std::is_same<decltype(detail::IsIterableImplementation<T1>(0)), std::true_type>::value &&
        std::is_same<decltype(detail::IsIterableImplementation<T2>(0)), std::true_type>::value,
    R>;

}  // namespace meta
}  // namespace fetch
