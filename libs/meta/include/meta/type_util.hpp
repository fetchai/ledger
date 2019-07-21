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

template <class T>
struct TypeConstant
{
  using type = T;
};
template <class T>
using TypeConstantT = typename TypeConstant<T>::type;  // technically, an identity mapping

template <template <class...> class F, class... List>
struct Accumulate;
template <template <class...> class F, class... List>
using AccumulateT = typename Accumulate<F, List...>::type;

template <template <class...> class F, class Last>
struct Accumulate<F, Last> : TypeConstant<Last>
{
};

template <template <class...> class F, class Car, class Cadr, class... Cddr>
struct Accumulate<F, Car, Cadr, Cddr...> : TypeConstant<AccumulateT<F, F<Car, Cadr>, Cddr...>>
{
};

template <class L, class R>
struct And : std::conditional_t<bool(L::value), R, L>
{
};
template <class L, class R>
static constexpr auto AndV = And<L, R>::value;

template <class... Ts>
using Conjunction = AccumulateT<And, std::true_type, Ts...>;

template <class... Ts>
static constexpr auto ConjunctionV = Conjunction<Ts...>::value;

template <template <class...> class F, class... Ts>
using All = Conjunction<F<Ts>...>;

template <template <class...> class F, class... Ts>
static constexpr auto AllV = All<F, Ts...>::value;

template <class L, class R>
struct Or : std::conditional_t<bool(L::value), L, R>
{
};
template <class L, class R>
static constexpr auto OrV = Or<L, R>::value;

template <class... Ts>
using Disjunction = AccumulateT<Or, std::false_type, Ts...>;

template <class... Ts>
static constexpr auto DisjunctionV = Disjunction<Ts...>::value;

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
using IsAnyOf = Disjunction<std::is_same<T, Ts>...>;

template <class T, class... Ts>
static constexpr auto IsAnyOfV = IsAnyOf<T, Ts...>::value;

template <class T, template <class...> class... Predicates>
using Satisfies = Conjunction<Predicates<T>...>;

template <class T, template <class...> class... Predicates>
static constexpr bool SatisfiesV = Satisfies<T, Predicates...>::value;

template <class T, template <class...> class... Predicates>
using SatisfiesAll = Satisfies<T, Predicates...>;

template <class T, template <class...> class... Predicates>
static constexpr bool SatisfiesAllV = SatisfiesAll<T, Predicates...>::value;

template <class...>
struct Switch;

template <class... Clauses>
using SwitchT = typename Switch<Clauses...>::type;

template <class If, class Then, class... Else>
struct Switch<If, Then, Else...> : std::conditional<If::value, Then, SwitchT<Else...>>
{
};

template <class Default>
struct Switch<Default> : TypeConstant<Default>
{
};

template <>
struct Switch<> : TypeConstant<void>
{
};

template <class Source, class Dest>
using CopyReferenceKind =
    Switch<std::is_lvalue_reference<Source>, std::add_lvalue_reference_t<Dest>,
           std::is_rvalue_reference<Source>, std::add_rvalue_reference_t<std::decay_t<Dest>>,
           std::decay_t<Dest>>;

template <class Source, class Dest>
using CopyReferenceKindT = typename CopyReferenceKind<Source, Dest>::type;

namespace detail_ {

template <class Arg>
constexpr decltype((std::declval<std::add_pointer_t<typename std::decay_t<Arg>::type>>(), true))
HasType(Arg &&) noexcept
{
  return true;
}
constexpr bool HasType(...) noexcept
{
  return false;
}

}  // namespace detail_

template <class Arg>
static constexpr bool HasMemberTypeV = detail_::HasType(std::declval<Arg>());

template <class Arg, bool = HasMemberTypeV<Arg>>
struct MemberType
{
  using type = typename Arg::type;
};
template <class Arg, bool b>
using MemberTypeT = typename MemberType<Arg, b>::type;

template <class Arg>
struct MemberType<Arg, false>
{
};

// Select<Type1, Type2...> provides member typedef type which is the leftmost Typen::type available.
// It is an error if none of template parameters has member type named type.
template <class... Clauses>
struct Select;
template <class... Clauses>
using SelectT = typename Select<Clauses...>::type;

template <class Car, class... Cdr>
struct Select<Car, Cdr...>
  : std::conditional_t<HasMemberTypeV<Car>, MemberType<Car>, Select<Cdr...>>
{
};

template <class... Ts>
struct Head;

template <class... Ts>
using HeadT = typename Head<Ts...>::type;

template <class... Ts>
struct Head : Accumulate<HeadT, Ts...> {};

template <class Car, class Cdr>
struct Head<Car, Cdr> : TypeConstant<Car>
{
};

template <class... Ts>
struct Last;

template <class... Ts>
using LastT = typename Last<Ts...>::type;

template <class... Ts>
struct Last: Accumulate<LastT, Ts...>;

template<class Car, class Cdr>
struct Last<Car, Cdr> : TypeConstant<Cdr>
{
};

template <class T>
struct HeadArgument;
template <class T>
using HeadArgumentT = typename HeadArgument<T>::type;

template <template <class...> class Ctor, class... Args>
struct HeadArgument<Ctor<Args...>> : Head<Args...>
{
};

template <class RV, class... Args>
struct HeadArgument<RV(Args...)> : Head<Args...>
{
};

template <class RV, class... Args>
struct HeadArgument<RV (&)(Args...)> : Head<Args...>
{
};

template <class RV, class... Args>
struct HeadArgument<RV (*)(Args...)> : Head<Args...>
{
};

template <class T>
struct ReturnZero
{
  static constexpr T Call() noexcept(std::is_nothrow_constructible<T>::value)
  {
    return T{};
  }
};

template <>
struct ReturnZero<void>
{
  static constexpr void Call() noexcept
  {}
};

}  // namespace type_util
}  // namespace fetch
