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

#include <cstddef>
#include <tuple>
#include <utility>

namespace fetch {
namespace meta {

template <typename T, typename... Ts>
struct RemoveFirstType;
template <typename T, typename... Ts>
struct RemoveFirstType<std::tuple<T, Ts...>>
{
  using type = typename std::tuple<Ts...>;
};

template <typename... Ts>
struct RemoveLastType;
template <typename... Ts>
struct RemoveLastType<std::tuple<Ts...>>
{
  template <std::size_t... Is>
  static std::tuple<std::tuple_element_t<Is, std::tuple<Ts...>>...> f(std::index_sequence<Is...>);
  using type = decltype(f(std::make_index_sequence<sizeof...(Ts) - 1u>()));
};

template <typename... Ts>
struct GetLastType;
template <typename... Ts>
struct GetLastType<std::tuple<Ts...>>
{
  using type = typename std::tuple_element_t<sizeof...(Ts) - 1u, std::tuple<Ts...>>;
};

}  // namespace meta
}  // namespace fetch
