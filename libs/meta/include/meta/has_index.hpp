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

#include <iterator>
#include <type_traits>
#include <utility>

namespace fetch {
namespace meta {

namespace detail {
template <class T, class Index>
struct HasIndexImpl
{
  template <class T1, class IndexDeduced = Index,
            class Reference = decltype((std::declval<T>())[std::declval<IndexDeduced>()]),
            class           = std::enable_if_t<!std::is_void<Reference>::value>>
  static std::true_type test(int);

  template <class>
  static std::false_type test(...);

  using Type = decltype(test<T>(0));
};

}  // namespace detail

template <typename T, typename Index>
constexpr bool HasIndex = detail::HasIndexImpl<T, Index>::Type::value;

template <typename T, typename R>
using IfHasIndex = std::enable_if_t<HasIndex<T, std::size_t>, R>;

template <typename T, typename R>
using IfHasNoIndex = std::enable_if_t<!HasIndex<T, std::size_t>, R>;

}  // namespace meta
}  // namespace fetch
