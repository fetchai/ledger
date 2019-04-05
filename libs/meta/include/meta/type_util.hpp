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
namespace type_util {

template <class... Ts>
struct Conjunction;

template <class... Ts>
static constexpr auto ConjunctionV = Conjunction<Ts...>::value;

template <class T, class... Ts>
struct Conjunction<T, Ts...>
{
  enum : bool
  {
    value = T::value && ConjunctionV<Ts...>
  };
};

template <>
struct Conjunction<>
{
  enum : bool
  {
    value = true
  };
};

template <template <class...> class F, class... Ts>
using All = Conjunction<F<Ts>...>;

template <template <class...> class F, class... Ts>
static constexpr auto AllV = All<F, Ts...>::value;

template <class... Ts>
struct Disjunction;

template <class... Ts>
static constexpr auto DisjunctionV = Disjunction<Ts...>::value;

template <class T, class... Ts>
struct Disjunction<T, Ts...>
{
  enum : bool
  {
    value = T::value || DisjunctionV<Ts...>
  };
};

template <>
struct Disjunction<>
{
  enum : bool
  {
    value = false
  };
};

template <template <class...> class F, class... Ts>
using Any = Disjunction<F<Ts>...>;

template <template <class...> class F, class... Ts>
static constexpr auto AnyV = Any<F, Ts...>::value;

template <template <class...> class F, class... Prefix>
struct Bind
{
  template <class... Args>
  using type = F<Prefix..., Args...>;
};

template <class T, class... Ts>
struct IsAnyOf
{
  enum : bool
  {
    value = AnyV<Bind<std::is_same, T>::template type, Ts...>
  };
};

template <class T, class... Ts>
static constexpr auto IsAnyOfV = IsAnyOf<T, Ts...>::value;

}  // namespace type_util
}  // namespace fetch
