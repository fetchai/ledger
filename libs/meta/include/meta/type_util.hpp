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
#include <utility>

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
using IsAnyOf = Any<Bind<std::is_same, T>::template type, Ts...>;

template <class T, class... Ts>
static constexpr auto IsAnyOfV = IsAnyOf<T, Ts...>::value;

template <class T, template <class...> class Predicate>
using Satisfies = Predicate<T>;

template <class T, template <class...> class Predicate>
static constexpr bool SatisfiesV = Satisfies<T, Predicate>::value;

template <class T, template <class...> class... Predicates>
using SatisfiesAll = Conjunction<Predicates<T>...>;

template <class T, template <class...> class... Predicates>
static constexpr bool SatisfiesAllV = SatisfiesAll<T, Predicates...>::value;

template <class F, class... Args>
struct IsNothrowInvocable
{
  enum : bool
  {
    value = noexcept(std::declval<F>()(std::declval<Args>()...))
  };
};

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

template <class...>
struct Switch;

template <class... Clauses>
using SwitchT = typename Switch<Clauses...>::type;

template <class If, class Then, class... Else>
struct Switch<If, Then, Else...> : std::conditional<If::value, Then, SwitchT<Else...>>
{
};

template <class Default>
struct Switch<Default>
{
  using type = Default;
};

template <>
struct Switch<>
{
  using type = void;
};

template <class Source, class Dest>
using CopyReferenceKind =
    Switch<std::is_lvalue_reference<Source>, std::add_lvalue_reference_t<Dest>,
           std::is_rvalue_reference<Source>, std::add_rvalue_reference_t<std::decay_t<Dest>>,
           std::decay_t<Dest>>;

template <class Source, class Dest>
using CopyReferenceKindT = typename CopyReferenceKind<Source, Dest>::type;

template <class...>
struct Head;

template <class... Ts>
using HeadT = typename Head<Ts...>::type;

template <class RV, class... Cdr>
struct Head<RV, Cdr...>
{
  using type = RV;
};

template <class...>
struct Last;

template <class... Ts>
using LastT = typename Last<Ts...>::type;

template <class Car, class Cadr, class... Cddr>
struct Last<Car, Cadr, Cddr...> : Last<Cadr, Cddr...>
{
};

template <class RV>
struct Last<RV>
{
  using type = RV;
};

}  // namespace type_util
}  // namespace fetch
