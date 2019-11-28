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

#include <functional>
#include <type_traits>
#include <utility>

namespace fetch {
namespace type_util {

template <typename...>
struct TypeBox;

template <typename T>
struct TypeBox<T>
{
  using type = T;
};

template <template <typename...> class F, typename... Prefix>
struct Bind
{
  template <typename... Args>
  using type = F<Prefix..., Args...>;
};

template <template <class...> class F, class... Args>
struct LeftAccumulate;

template <template <class...> class F, class... Args>
using LeftAccumulateT = typename LeftAccumulate<F, Args...>::type;

template <template <class...> class F, class A1, class A2, class... As>
struct LeftAccumulate<F, A1, A2, As...> : LeftAccumulate<F, F<A1, A2>, As...>
{
};

template <template <class...> class F, class RV>
struct LeftAccumulate<F, RV> : TypeBox<RV>
{
};

template <class F>
struct StaticOperator
{
  template <class... Ts>
  struct type
  {
    static constexpr auto value = F{}(Ts::value...);
  };
};

template <template <class...> class F>
using StlOperator = StaticOperator<F<void>>;

template <typename... Ts>
using Conjunction =
    LeftAccumulateT<StlOperator<std::logical_and>::template type, std::true_type, Ts...>;

template <typename... Ts>
static constexpr auto ConjunctionV = Conjunction<Ts...>::value;

template <template <typename...> class F, typename... Ts>
using All = Conjunction<F<Ts>...>;

template <template <typename...> class F, typename... Ts>
static constexpr auto AllV = All<F, Ts...>::value;

template <typename... Ts>
using Disjunction =
    LeftAccumulateT<StlOperator<std::logical_or>::template type, std::false_type, Ts...>;

template <typename... Ts>
static constexpr auto DisjunctionV = Disjunction<Ts...>::value;

template <template <typename...> class F, typename... Ts>
using Any = Disjunction<F<Ts>...>;

template <typename T, typename... Ts>
using IsAnyOf = Disjunction<std::is_same<T, Ts>...>;

template <typename T, typename... Ts>
static constexpr auto IsAnyOfV = IsAnyOf<T, Ts...>::value;

template <typename T, template <typename...> class... Predicates>
using SatisfiesAll = Conjunction<Predicates<T>...>;

template <typename T, template <typename...> class... Predicates>
static constexpr bool SatisfiesAllV = SatisfiesAll<T, Predicates...>::value;

template <typename F, typename... Args>
struct InvokeResult : TypeBox<decltype(std::declval<F>()(std::declval<Args>()...))>
{
};

template <typename F, typename... Args>
using InvokeResultT = typename InvokeResult<F, Args...>::type;

template <class T, class... Ts>
struct AreSimilar : Conjunction<AreSimilar<T, Ts>...>
{
};

template <class T, class... Ts>
static constexpr auto AreSimilarV = AreSimilar<T, Ts...>::value;

template <class T1, class T2>
struct AreSimilar<T1, T2> : std::is_same<T1, T2>
{
};

template <template <class...> class F, class... Args1, class... Args2>
struct AreSimilar<F<Args1...>, F<Args2...>> : std::true_type
{
};

namespace tuple {

template <class... Tuples>
struct Concat : LeftAccumulateT<Concat, std::tuple<>, std::decay_t<Tuples>...>
{
};

template <class... Tuples>
using ConcatT = typename Concat<Tuples...>::type;

template <class... Ts1, class... Ts2>
struct Concat<std::tuple<Ts1...>, std::tuple<Ts2...>> : TypeBox<std::tuple<Ts1..., Ts2...>>
{
};

template <class Car, class Cdr>
using Cons = Concat<std::tuple<Car>, Cdr>;

template <class Car, class Cdr>
using ConsT = typename Cons<Car, Cdr>::type;

template <class Init, class Last>
using Append = Concat<Init, std::tuple<Last>>;

template <class Init, class Last>
using AppendT = typename Append<Init, Last>::type;

}  // namespace tuple
}  // namespace type_util
}  // namespace fetch
