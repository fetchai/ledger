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

// Throughout this header all the variadic templates have one peculiar property:
// if a template parameter is pack::Pack, then the template is applied to its elements
// rather than the pack itself.
// So, for instance, if F is a binary template
//
//     template<class, class> class F;
//
// then
//
//     AccumulateT<F, int, pack::Pack<char, std::true_type>, double>
//
// is actually
//
//     F<F<F<int, char>, std::true_type>, double>
//
// char and std::true_type are treated the same way as the two other parameters.
// Or, say, Switches can be written as
//
//     Switch<pack::Pack<If1, Then1>, pack::Pack<If2, Then2>, pack::Pack<If3, Then3>>
//
// for greater readability.

// TypeConstant<T> is an otherwise empty structure whose sole member is a typedef type = T.
template <class T>
using TypeConstant = pack::Constant<T>;
template <class T>
using TypeConstantT = typename TypeConstant<T>::type;  // technically, an identity mapping

// Accumulate<F, Ts...> is a left fold of F over Ts...
// The pack should be non-empty.
template <template <class...> class F, class... List>
using Accumulate = pack::Accumulate<F, pack::ConcatT<List...>>;

template <template <class...> class F, class... List>
using AccumulateT = typename Accumulate<F, List...>::type;

template <template <class...> class F, class... List>
static constexpr auto AccumulateV = Accumulate<F, List...>::value;

// And<A, B> is a lispish binary operator && on types.
// Its result is a true type if both A and B are.
// A::value should be defined and convertible to bool.
// No requirements imposed on B.
template <class A, class B>
using And = pack::And<A, B>;

template <class A, class B>
static constexpr auto AndV = And<A, B>::value;

// Conjunction<List...> is a true type iff every element of List... is.
template <class... Ts>
using Conjunction = pack::Conjunction<pack::ConcatT<Ts...>>;

template <class... Ts>
static constexpr auto ConjunctionV = Conjunction<Ts...>::value;

// All<F, List...> is a true type iff F applied to each element of List... returns a true type.
template <template <class...> class F, class... Ts>
using All = pack::All<F, pack::ConcatT<Ts...>>;

template <template <class...> class F, class... Ts>
static constexpr auto AllV = All<F, Ts...>::value;

// Or<A, B> is a lispish binary operator || on types.
// Its result is a true type if either A or B is.
// A::value should be defined and convertible to bool.
// No requirements imposed on B.
template <class A, class B>
using Or = pack::Or<A, B>;

template <class A, class B>
static constexpr auto OrV = Or<A, B>::value;

// Disjunction<List...> is a true type iff at least one element of List... is.
template <class... Ts>
using Disjunction = pack::Disjunction<pack::ConcatT<Ts...>>;

template <class... Ts>
static constexpr auto DisjunctionV = Disjunction<Ts...>::value;

// Any<F, List...> is a true type iff F applied to at least one element of List... returns a true
// type.
template <template <class...> class F, class... Ts>
using Any = pack::Any<F, pack::ConcatT<Ts...>>;

template <template <class...> class F, class... Ts>
static constexpr auto AnyV = Any<F, Ts...>::value;

// Bind<F, Args...> is an otherwise empty structure whose sole member is a template type such that
// Bind<F, T...>::template type<U...> is the same as F<T..., U...>.
// Bind is the only type-containing struct defined in this header that does not have accompanying
// BindT, current C++ wouldn't allow that.
template <template <class...> class F, class... Prefix>
using Bind = pack::Bind<F, pack::ConcatT<Prefix...>>;

// IsAnyOf<T, List...> is a true type if T is equal, in the sense of std::is_same, to at least one
// element of List...
template <class T, class... Ts>
using IsAnyOf = pack::IsAnyOf<T, pack::ConcatT<Ts...>>;

template <class T, class... Ts>
static constexpr auto IsAnyOfV = IsAnyOf<T, Ts...>::value;

// Satisfies<T, Predicates...> is a true type if Predicate<T> is a true type for each of the
// Predicates.
template <class T, template <class...> class... Predicates>
using Satisfies = Conjunction<Predicates<T>...>;

template <class T, template <class...> class... Predicates>
static constexpr bool SatisfiesV = Satisfies<T, Predicates...>::value;

// IsInvocable and InvokeResult are limited backports of their C++17's namesakes.
// IsNothrowInvocable can hardly be reasonably portably implemented in C++14.
template <class F, class... Args>
using IsInvocable = pack::IsInvocable<F, pack::ConcatT<Args...>>;
template <class F, class... Args>
static constexpr auto IsInvocableV = IsInvocable<F, Args...>::value;

template <class F, class... Args>
using InvokeResult = pack::InvokeResult<F, pack::ConcatT<Args...>>;
template <class F, class... Args>
using InvokeResultT = typename InvokeResult<F, Args...>::type;

// Switch<Clauses...> implements top-down linear switch.
// Its arguments are split in coupled pairs: Condition, Then-Expression.
// Its member typedef type is equal to the leftmost Then-Expression
// whose preceeding Condition is a true type. If none of the Conditions is true then
// the number of elements in Clauses should be odd, and its last element is taken as the default
// statement.
//     SwitchT<std::false_type, int, SizeConstant<0>, double, std::true_type, std::string, char> is
//     std::string; SwitchT<std::false_type, int, SizeConstant<0>, double, char> is char.
// If all the Conditions are false and there's no Default statement, Switch::type is void.
template <class... Clauses>
using Switch = pack::Switch<pack::ConcatT<Clauses...>>;

template <class... Clauses>
using SwitchT = typename Switch<Clauses...>::type;

// BinarySwitch allows to perform run-time dispatch of isomorphic templated constructs based on an
// index value. It is similar to a logarithmic lookup-optimized switch statements but allows to wrap
// identical operations on values of different types in a templated callable, thus potentially
// reducing code duplication. See type_util_tests.cpp for an example; or refactoring of
// type-dispatches in vm in an upcoming PR.
namespace detail_ {

// ReturnZero<T>::Call() returns value-constructed value of type T, and is a no-op for T = void.
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

// SwitchNode is the intermediary node in binary search tree.
// Its branches are expected to be sorted left-to-right, in that every Id handled by Left is less
// (in the sense of member ::value comparison) than every Id handled by Right.
template <class Left, class Right>
struct SwitchNode
{
  /**
   * Returns LowerBound(), that is the least Id that can be handled by this Node.
   *
   * @return the least Id that can be handled by this Node
   */
  static constexpr auto LowerBound() noexcept(noexcept(Left::LowerBound()))
  {
    return Left::LowerBound();
  }

  /**
   * Returns whether an Id can be handled by this Node.
   * For non-leaf nodes this is simply a check if this Id is not less than Node's LowerBound().
   * Which, in turn implies that on Call()ing this node, Id is always checked left-to-right, Right
   * branch first, as its Ids are greater than those of Left.
   *
   * @param Id probably an integral-valued class, matched against this node's lower bound
   * @return false if Id is too low to be handled by this Node, true otherwise.
   */
  template <class Id>
  static constexpr bool CanHandle(Id) noexcept(noexcept(LowerBound() <= Id::value))
  {
    return LowerBound() <= Id::value;
  }

  /**
   * Probably invokes a function over arguments.
   * This method should only be handled if `CanHandle()` returned true for this Id.
   *
   * @param selector value to be matched against BinarySwitch's set of known types
   * @param f function to be called on particular args adapted through a particular Id that matched
   * selector
   * @param args
   * @return whatever the appropriate Switch branch returned
   */
  template <class Id, class F, class... Args>
  static constexpr decltype(auto) Call(Id selector, F &&f, Args &&... args) noexcept(
      noexcept(Right::CanHandle(std::declval<Id>())) &&
      noexcept(Right::Call(std::declval<Id>(), std::declval<F>(), std::declval<Args>()...)) &&
      noexcept(Left::CanHandle(std::declval<Id>())) &&
      noexcept(Left::Call(std::declval<Id>(), std::declval<F>(), std::declval<Args>()...)) &&
      noexcept(ReturnZero<decltype(Right::Call(std::declval<Id>(), std::declval<F>(),
                                               std::declval<Args>()...))>::Call()))

  {
    // as nodes check ids by comparing them against lower bound, first check the right branch
    if (Right::CanHandle(selector))
    {
      return Right::Call(selector, std::forward<F>(f), std::forward<Args>(args)...);
    }
    if (Left::CanHandle(selector))
    {
      return Left::Call(selector, std::forward<F>(f), std::forward<Args>(args)...);
    }
    // if none of the branches can handle this id, return zero value
    // TODO(bipll): it can make sense to introduce a default handler
    return ReturnZero<decltype(
        Right::Call(selector, std::forward<F>(f), std::forward<Args>(args)...))>::Call();
  }
};

// The leaf node, where the invocation itself is done, if Id matches.
template <class ParticularId>
struct SwitchLeaf
{
  /**
   * Returns this leaf's lower bound. Unlike in branch nodes, leaf can only handle ParticularId.
   *
   * @return ParticularId's value
   */
  static constexpr auto LowerBound() noexcept
  {
    return ParticularId::value;
  }

  /**
   * Checks id Id corresponds to this Leaf's id.
   *
   * @param Id an integral_sequence-like type
   * @return true iff this leaf is a match for Id, i.e. when Id is value-equal to ParticularId
   */
  template <class Id>
  static constexpr bool CanHandle(Id) noexcept(noexcept(Id::value == LowerBound()))
  {
    return Id::value == LowerBound();
  }

  /**
   * Invokes a function over arguments.
   * This method should only be handled if `CanHandle()` returned true for this Id.
   *
   * @param Id value to be matched against BinarySwitch's set of known types, here ignored
   * @param f function to be called on particular args adapted through a particular Id that matched
   * selector
   * @param args
   * @return whatever the appropriate Switch branch returned
   */
  template <class Id, class F, class... Args>
  static constexpr decltype(auto) Call(Id, F &&f, Args &&... args) noexcept(
      noexcept(std::forward<F>(f)(ParticularId(std::forward<Args>(args))...)))
  {
    // no id match check is done here
    return std::forward<F>(f)(ParticularId(std::forward<Args>(args))...);
  }
};

// By default, a BinarySwitch implementation is a node with BinarySwitches at both branches, each
// one handling its half of the ids.
template <class Ids>
struct BinarySwitch
  : SwitchNode<BinarySwitch<pack::LeftHalfT<Ids>>, BinarySwitch<pack::RightHalfT<Ids>>>
{
};

// If there's only one id to handle, it's a leaf.
template <class ParticularId>
struct BinarySwitch<pack::Pack<ParticularId>> : SwitchLeaf<ParticularId>
{
};

}  // namespace detail_

// Ids... are packed in a pack::Pack and sorted in ascending order, duplicates removed.
template <class... Ids>
using BinarySwitch = detail_::BinarySwitch<pack::UniqueSortT<pack::ConcatT<Ids...>>>;

// This one may prove useful, so that we can convert a single integral_sequence (or a similar
// template instantiation) into a pack of singleton integer_sequences, to be used as case
// alternatives for BinarySwitch.
template <class Sequence>
struct LiftIntegerSequence;
template <class Sequence>
using LiftIntegerSequenceT = typename LiftIntegerSequence<Sequence>::type;
template <template <class Id, Id...> class Ctor, class Id, Id... ids>
struct LiftIntegerSequence<Ctor<Id, ids...>>
  : TypeConstant<pack::Pack<std::integer_sequence<Id, ids>...>>
{
};

// CopyReferenceKind<A, B> provide a member typedef type that is B with ref-qualifier changed to
// that of A.
template <class Source, class Dest>
using CopyReferenceKind =
    Switch<std::is_lvalue_reference<Source>, std::add_lvalue_reference_t<Dest>,
           std::is_rvalue_reference<Source>, std::add_rvalue_reference_t<std::decay_t<Dest>>,
           std::decay_t<Dest>>;

template <class Source, class Dest>
using CopyReferenceKindT = typename CopyReferenceKind<Source, Dest>::type;

// HasMemberTypeV<T> is true if T::type is a public type of kind *, and false otherwise.
template <class Arg>
static constexpr bool HasMemberTypeV = pack::HasMemberTypeV<Arg>;

// MemberType<T> is an otherwise empty structure whose sole member is a typedef type = T::type.
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

// Head<Pack> evaluates to Pack's first element.
// It is an error if Head is applied to Nil.
template <class... Ts>
using Head = pack::Head<pack::ConcatT<Ts...>>;

template <class... Ts>
using HeadT = typename Head<Ts...>::type;

// Last<Pack> evaluates to Pack's last element.
// It is an error if Last is applied to Nil.
template <class... Ts>
using Last = pack::Last<pack::ConcatT<Ts...>>;

template <class... Ts>
using LastT = typename Last<Ts...>::type;

// HeadArgument provides member typedef type that is equal to the first argument of a template
// instantiation or a function.
template <class T>
using HeadArgument = pack::Head<pack::ArgsT<T>>;

template <class T>
using HeadArgumentT = typename HeadArgument<T>::type;

}  // namespace type_util
}  // namespace fetch
