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

#include <type_traits>
#include <utility>

namespace fetch {
namespace type_util {

template <typename... Ts>
struct Conjunction;

template <typename... Ts>
static constexpr auto ConjunctionV = Conjunction<Ts...>::value;

template <typename T, typename... Ts>
struct Conjunction<T, Ts...>
{
  static constexpr bool value = T::value && ConjunctionV<Ts...>;
};

template <>
struct Conjunction<>
{
  static constexpr bool value = true;
};

template <template <typename...> class F, typename... Ts>
using All = Conjunction<F<Ts>...>;

template <template <typename...> class F, typename... Ts>
static constexpr auto AllV = All<F, Ts...>::value;

template <typename... Ts>
struct Disjunction;

template <typename... Ts>
static constexpr auto DisjunctionV = Disjunction<Ts...>::value;

template <typename T, typename... Ts>
struct Disjunction<T, Ts...>
{
  static constexpr bool value = T::value || DisjunctionV<Ts...>;
};

template <>
struct Disjunction<>
{
  static constexpr bool value = false;
};

template <template <typename...> class F, typename... Ts>
using Any = Disjunction<F<Ts>...>;

template <template <typename...> class F, typename... Prefix>
struct Bind
{
  template <typename... Args>
  using type = F<Prefix..., Args...>;
};

template <typename T, typename... Ts>
using IsAnyOf = Any<Bind<std::is_same, T>::template type, Ts...>;

template <typename T, typename... Ts>
static constexpr auto IsAnyOfV = IsAnyOf<T, Ts...>::value;

template <typename T, template <typename...> class... Predicates>
using SatisfiesAll = Conjunction<Predicates<T>...>;

template <typename T, template <typename...> class... Predicates>
static constexpr bool SatisfiesAllV = SatisfiesAll<T, Predicates...>::value;

template <typename F, typename... Args>
struct IsNothrowInvocable
{
  static constexpr bool value = noexcept(std::declval<F>()(std::declval<Args>()...));
};

template <typename F, typename... Args>
static constexpr auto IsNothrowInvocableV = IsNothrowInvocable<F, Args...>::value;

template <typename F, typename... Args>
struct InvokeResult
{
  using type = decltype(std::declval<F>()(std::declval<Args>()...));
};

template <typename F, typename... Args>
using InvokeResultT = typename InvokeResult<F, Args...>::type;

}  // namespace type_util
}  // namespace fetch
