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

#include "meta/pack.hpp"

#include <type_traits>
#include <utility>

namespace fetch {
namespace type_util {

template <class T>
using TypeConstant = pack::Constant<T>;
template <class T>
using TypeConstantT = typename TypeConstant<T>::type;  // technically, an identity mapping

template <template <class...> class F, class... List>
using Accumulate = pack::Accumulate<F, pack::ConcatT<List...>>;

template <template <class...> class F, class... List>
using AccumulateT = typename Accumulate<F, List...>::type;

template <template <class...> class F, class... List>
static constexpr auto AccumulateV = Accumulate<F, List...>::value;

template <class A, class B>
using And = pack::And<A, B>;

template <class A, class B>
static constexpr auto AndV = And<A, B>::value;

template <class... Ts>
using Conjunction = pack::Conjunction<pack::ConcatT<Ts...>>;

template <class... Ts>
static constexpr auto ConjunctionV = Conjunction<Ts...>::value;

template <template <class...> class F, class... Ts>
using All = pack::All<F, pack::ConcatT<Ts...>>;

template <template <class...> class F, class... Ts>
static constexpr auto AllV = All<F, Ts...>::value;

template <class A, class B>
using Or = pack::Or<A, B>;

template <class A, class B>
static constexpr auto OrV = Or<A, B>::value;

template <class... Ts>
using Disjunction = pack::Disjunction<pack::ConcatT<Ts...>>;

template <class... Ts>
static constexpr auto DisjunctionV = Disjunction<Ts...>::value;

template <template <class...> class F, class... Ts>
using Any = pack::Any<F, pack::ConcatT<Ts...>>;

template <template <class...> class F, class... Ts>
static constexpr auto AnyV = Any<F, Ts...>::value;

template <template <class...> class F, class... Prefix>
using Bind = pack::Bind<F, pack::ConcatT<Prefix...>>;

template <class T, class... Ts>
using IsAnyOf = pack::IsAnyOf<T, pack::ConcatT<Ts...>>;

template <class T, class... Ts>
static constexpr auto IsAnyOfV = IsAnyOf<T, Ts...>::value;

template <class T, template <class...> class... Predicates>
using Satisfies = Conjunction<Predicates<T>...>;

template <class T, template <class...> class... Predicates>
static constexpr bool SatisfiesV = Satisfies<T, Predicates...>::value;

template <class F, class... Args>
using IsInvocable = pack::IsInvocable<F, pack::ConcatT<Args...>>;
template <class F, class... Args>
static constexpr auto IsInvocableV = IsInvocable<F, Args...>::value;

template <class F, class... Args>
using InvokeResult = pack::InvokeResult<F, pack::ConcatT<Args...>>;
template <class F, class... Args>
using InvokeResultT = typename InvokeResult<F, Args...>::type;

template <class... Clauses>
using Switch = pack::Switch<pack::ConcatT<Clauses...>>;

template <class... Clauses>
using SwitchT = typename Switch<Clauses...>::type;

namespace detail_ {

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

template <class Left, class Right>
struct SwitchNode
{
  static constexpr auto LowerBound() noexcept(noexcept(Left::LowerBound()))
  {
    return Left::LowerBound();
  }
  template <class Id>
  static constexpr bool CanHandle(Id) noexcept(noexcept(LowerBound() <= Id::value))
  {
    return LowerBound() <= Id::value;
  }
  template <class Id, class F, class... Args>
  static constexpr decltype(auto) Call(Id selector, F &&f, Args &&... args) noexcept(
      noexcept(Right::CanHandle(std::declval<Id>())) &&
      noexcept(Right::Call(std::declval<Id>(), std::declval<F>(), std::declval<Args>()...)) &&
      noexcept(Left::CanHandle(std::declval<Id>())) &&
      noexcept(Left::Call(std::declval<Id>(), std::declval<F>(), std::declval<Args>()...)) &&
      noexcept(ReturnZero<decltype(Right::Call(std::declval<Id>(), std::declval<F>(),
                                               std::declval<Args>()...))>::Call()))

  {
    if (Right::CanHandle(selector))
    {
      return Right::Call(selector, std::forward<F>(f), std::forward<Args>(args)...);
    }
    if (Left::CanHandle(selector))
    {
      return Left::Call(selector, std::forward<F>(f), std::forward<Args>(args)...);
    }
    return ReturnZero<decltype(
        Right::Call(selector, std::forward<F>(f), std::forward<Args>(args)...))>::Call();
  }
};

template <class ParticularId>
struct SwitchLeaf
{
  static constexpr auto LowerBound() noexcept
  {
    return ParticularId::value;
  }
  template <class Id>
  static constexpr bool CanHandle(Id) noexcept(noexcept(Id::value == LowerBound()))
  {
    return Id::value == LowerBound();
  }
  template <class Id, class F, class... Args>
  static constexpr decltype(auto) Call(Id, F &&f, Args &&... args) noexcept(
      noexcept(std::declval<F>()(ParticularId(std::declval<Args>())...)))
  {
    // no id match check is done here
    return std::forward<F>(f)(ParticularId(std::forward<Args>(args))...);
  }
};

template <class Ids>
struct BinarySwitch
  : SwitchNode<BinarySwitch<pack::LeftHalfT<Ids>>, BinarySwitch<pack::RightHalfT<Ids>>>
{
};

template <class ParticularId>
struct BinarySwitch<pack::Pack<ParticularId>> : SwitchLeaf<ParticularId>
{
};

}  // namespace detail_

template <class... Ids>
using BinarySwitch = detail_::BinarySwitch<pack::UniqueSortT<pack::ConcatT<Ids...>>>;

template <class Sequence>
struct LiftIntegerSequence;
template <class Sequence>
using LiftIntegerSequenceT = typename LiftIntegerSequence<Sequence>::type;
template <class Id, Id... ids>
struct LiftIntegerSequence<std::integer_sequence<Id, ids...>>
  : TypeConstant<pack::Pack<std::integer_sequence<Id, ids>...>>
{
};

template <class Source, class Dest>
using CopyReferenceKind =
    Switch<std::is_lvalue_reference<Source>, std::add_lvalue_reference_t<Dest>,
           std::is_rvalue_reference<Source>, std::add_rvalue_reference_t<std::decay_t<Dest>>,
           std::decay_t<Dest>>;

template <class Source, class Dest>
using CopyReferenceKindT = typename CopyReferenceKind<Source, Dest>::type;

template <class Arg>
static constexpr bool HasMemberTypeV = pack::HasMemberTypeV<Arg>;

template <class Arg>
using MemberType = pack::MemberType<Arg>;

template <class Arg>
using MemberTypeT = typename MemberType<Arg>::type;

// Select<Type1, Type2...> provides member typedef type which is the leftmost Typen::type available.
// It is an error if none of template parameters has member type named type.
template <class... Clauses>
using Select = pack::Select<pack::ConcatT<Clauses...>>;

template <class... Clauses>
using SelectT = typename Select<Clauses...>::type;

template <class... Ts>
using Head = pack::Head<pack::ConcatT<Ts...>>;

template <class... Ts>
using HeadT = typename Head<Ts...>::type;

template <class... Ts>
using Last = pack::Last<pack::ConcatT<Ts...>>;

template <class... Ts>
using LastT = typename Last<Ts...>::type;

template <class T>
using HeadArgument = pack::Head<pack::ArgsT<T>>;

template <class T>
using HeadArgumentT = typename HeadArgument<T>::type;

}  // namespace type_util
}  // namespace fetch
