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
// rather than to the pack itself.
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
// Or, say, Cases can be written as
//
//     Case<pack::Pack<If1, Then1>, pack::Pack<If2, Then2>, pack::Pack<If3, Then3>>
//
// for greater readability.

/**
 * A simple function that wraps its argument in a structure with a sole member typedef type.
 *
 * @param T an arbitrary type
 * @return structure S such that S::type is T
 */
template <class T>
using TypeConstant = pack::Constant<T>;
template <class T>
using TypeConstantT = typename TypeConstant<T>::type;  // technically, an identity mapping

// Two useful partial specializations of std::integral_constant.
/**
 * Returns an std::integral_constant of type std::size_t.
 *
 * @param i a compile-time size constant
 * @return std::integral_constant<std::size_t, i>
 */
template <std::size_t i>
using SizeConstant = pack::SizeConstant<i>;

/**
 * Returns an std::integral_constant of type bool.
 *
 * @param b a compile-time boolean constant
 * @return std::integral_constant<bool, i>
 */
template <bool b>
using BoolConstant = pack::BoolConstant<b>;

/**
 * Left fold of a function over a pack.
 * Zero value is taken from the pack itself, so the latter should be non-empty.
 * (Similar to foldl1 in Prelude.PreludeList.)
 *
 * @param F an arbitrary class template
 * @param List... a non-empty pack of arbitrary types T0, T1, ..., Tn
 * @return F<F< ... <F<T0, T1>, ... >, Tn>
 */
template <template <class...> class F, class... List>
using Accumulate = pack::Accumulate<F, pack::ConcatT<List...>>;

template <template <class...> class F, class... List>
using AccumulateT = typename Accumulate<F, List...>::type;

template <template <class...> class F, class... List>
static constexpr auto AccumulateV = Accumulate<F, List...>::value;

/**
 * A binary operator && on types.
 *
 * @param A a bool-valued type
 * @param B an arbitrary type
 * @return B if A is a true type; A otherwise
 */
template <class A, class B>
using And = pack::And<A, B>;

template <class A, class B>
static constexpr auto AndV = And<A, B>::value;

/**
 * A lisp-style operator && accepting an arbitrary amount of arguments an returning the leftmost
 * false of them, or the very last one if all others are true. Conjunction of an empty pack is a
 * true type. Warning: ConjunctionT returns a member type type of the leftmost false type, not that
 * latter itself.
 *
 * @param Ts... arbitrary types
 * @return the leftmost false of Ts..., or the last one, if none
 */
template <class... Ts>
using Conjunction = pack::Conjunction<pack::ConcatT<Ts...>>;

template <class... Ts>
static constexpr auto ConjunctionV = Conjunction<Ts...>::value;

/**
 * Check if every element of a pack satisfies a predicate.
 *
 * @param F a class template that evaluates to a bool-valued class on each element of the pack
 * (until one of them breaks the predicate, at least)
 * @param Ts... arbitrary types
 * @return
 */
template <template <class...> class F, class... Ts>
using All = pack::All<F, pack::ConcatT<Ts...>>;

template <template <class...> class F, class... Ts>
static constexpr auto AllV = All<F, Ts...>::value;

/**
 * A binary operator || on types.
 *
 * @param A a bool-valued type
 * @param B an arbitrary type
 * @return A if A is a true type; B otherwise
 */
template <class A, class B>
using Or = pack::Or<A, B>;

template <class A, class B>
static constexpr auto OrV = Or<A, B>::value;

/**
 * A lisp-style operator || accepting an arbitrary amount of arguments an returning the leftmost
 * true of them, or the very last one if all others are false. Disjunction of an empty pack is a
 * false type. Warning: DisjunctionT returns a member type type of the leftmost true type, not that
 * latter itself.
 *
 * @param Ts... arbitrary types
 * @return the leftmost true of Ts..., or the last one, if none
 */
template <class... Ts>
using Disjunction = pack::Disjunction<pack::ConcatT<Ts...>>;

template <class... Ts>
static constexpr auto DisjunctionV = Disjunction<Ts...>::value;

/**
 * Check if at least one element of a pack satisfies a predicate.
 *
 * @param F a class template that evaluates to a bool-valued class on each element of the pack
 * (until one of them satisfies the predicate, at least)
 * @param Ts... arbitrary types
 * @return
 */
template <template <class...> class F, class... Ts>
using Any = pack::Any<F, pack::ConcatT<Ts...>>;

template <template <class...> class F, class... Ts>
static constexpr auto AnyV = Any<F, Ts...>::value;

/**
 * Partial function application.
 * Note that the result should be used as Bind<F, Args>::template type. Consequently, BindT is not
 * defined.
 *
 * @param F an arbitrary class template
 * @param Prefix... arbitrary types T0, ..., Tn, n is less than F's maximum arity
 * @return struct S such that S::template type<Args...> is exactly F<T0, ..., Tn, Args...>
 */
template <template <class...> class F, class... Prefix>
using Bind = pack::Bind<F, pack::ConcatT<Prefix...>>;

/**
 * Check if type is found in a pack.
 *
 * @param T an arbitrary type
 * @param Ts... abitrary types
 * @return true type iff there's a type among Ts... equal to T, in the sense of std::is_same
 */
template <class T, class... Ts>
using IsAnyOf = pack::IsAnyOf<T, pack::ConcatT<Ts...>>;

template <class T, class... Ts>
static constexpr auto IsAnyOfV = IsAnyOf<T, Ts...>::value;

/**
 * Check if a type satisfies every given predicate
 *
 * @param T an arbitrary type
 * @param Predicates... arbitrary class templates returning bool-valued types on T
 * @return true type iff T satisfies all Predicates...
 */
template <class T, template <class...> class... Predicates>
using Satisfies = Conjunction<Predicates<T>...>;

template <class T, template <class...> class... Predicates>
static constexpr bool SatisfiesV = Satisfies<T, Predicates...>::value;

// IsInvocable and InvokeResult are limited backports of their C++17's namesakes.
// IsNothrowInvocable can hardly be reasonably portably implemented in C++14.
/**
 * Check if a function (here, the real C++ function) can be called with arguments of such types.
 *
 * @param F a function type
 * @param P a Pack<T0...> of argument types
 * @return true type if f(t0...) is a valid expression, where f is of type F and tn is of type Tn,
 * for each n
 */
template <class F, class... Args>
using IsInvocable = pack::IsInvocable<F, pack::ConcatT<Args...>>;
template <class F, class... Args>
static constexpr auto IsInvocableV = IsInvocable<F, Args...>::value;

/**
 * Get the result type of an invocation of a function (here, the real C++ function) over arguments
 * of such types. Warning: IsInvocable<F, P> should be a true type.
 *
 * @param F a function type
 * @param P a Pack<T0...> of argument types
 * @return struct S such that f(t0...) is of type S::type, where f is of type F and tn is of type
 * Tn, for each n
 */
template <class F, class... Args>
using InvokeResult = pack::InvokeResult<F, pack::ConcatT<Args...>>;
template <class F, class... Args>
using InvokeResultT = typename InvokeResult<F, Args...>::type;

/**
 * Top-down linear switch.
 * Arguments are split in coupled pairs: Condition, Then-Expression.
 * The result has member typedef type equal to the leftmost Then-Expression
 * whose preceeding Condition is a true type. If none of the Conditions is true then
 * the number of elements in Clauses should be odd, and its last element is taken as the default
 * statement.
 *     CaseT<
 *         std::false_type, int,
 *         SizeConstant<0>, double,
 *         std::true_type, std::string,
 *         char>
 *     is
 *         std::string;
 *
 *     CaseT<
 *         std::false_type, int,
 *         SizeConstant<0>, double,
 *         char>
 *     is
 *         char.
 * If all the Conditions are false and there's no Default statement, Case::type is void.
 *
 * @param Clauses... a sequence of bool-valued and arbitrary types, interleaved (the rightmost one needs not to be bool-valued)
 * @return
 */
template <class... Clauses>
using Case = pack::Case<pack::ConcatT<Clauses...>>;

template <class... Clauses>
using CaseT = typename Case<Clauses...>::type;

/**
 * Transfer reference-kind from a source type to the destination type:
 *
 *     CopyReferenceKindT<int &, std::string>        is std::string &
 *     CopyReferenceKindT<int &&, const std::string> is std::string const &&
 *     CopyReferenceKindT<int, std::string>          is std::string
 *
 * @param Source an arbitrary type that specifies reference kind
 * @param Dest an arbitrary type to be converted into a proper reference type
 * @return
 */
template <class Source, class Dest>
using CopyReferenceKind =
Case<std::is_lvalue_reference<Source>, std::add_lvalue_reference_t<Dest>,
	std::is_rvalue_reference<Source>, std::add_rvalue_reference_t<std::decay_t<Dest>>,
	std::decay_t<Dest>>;

template <class Source, class Dest>
using CopyReferenceKindT = typename CopyReferenceKind<Source, Dest>::type;

/**
 * Detect if its argument has member type type.
 *
 * @param T an arbitrary type
 * @return true iff T is a class type with public member type type of kind *
 */
template <class Arg>
static constexpr bool HasMemberTypeV = pack::HasMemberTypeV<Arg>;

/**
 * Extract argument's member type type and wrap it back in a struct.
 *
 * @param T an arbitrary type
 * @return struct S such that S::type is T::type if the latter is a public type of kind *; empty
 * struct otherwise
 */
template <class Arg>
using MemberType = pack::MemberType<Arg>;

template <class Arg>
using MemberTypeT = typename MemberType<Arg>::type;

/**
 * Select the leftmost member type type defined in pack elements.
 *
 *     struct A { using type = int; };
 *
 *     Select<char, double, A, std::string>
 *         is int.
 *
 * @param Clauses... arbitrary types
 * @return
 */
template <class... Clauses>
using Select = pack::Select<pack::ConcatT<Clauses...>>;

template <class... Clauses>
using SelectT = typename Select<Clauses...>::type;

/**
 * Get the first element of a pack.
 *
 * @param Ts... a (non-empty) pack of arbitrary types
 * @return the very leftmost of Ts...
 */
template <class... Ts>
using Head = pack::Head<pack::ConcatT<Ts...>>;

template <class... Ts>
using HeadT = typename Head<Ts...>::type;

/**
 * Get pack's rightmost element, symmetric to Head.
 *
 * @param Ts... a (non-empty) pack of arbitrary types
 * @return
 */
template <class... Ts>
using Last = pack::Last<pack::ConcatT<Ts...>>;

template <class... Ts>
using LastT = typename Last<Ts...>::type;

/**
 * Get the first argument type of a function type, or a template instantiation.
 *
 * @param T either a free/member function type, or a template instantiation
 * @return
 */
template <class T>
using HeadArgument = pack::Head<pack::ArgsT<T>>;

template <class T>
using HeadArgumentT = typename HeadArgument<T>::type;

}  // namespace type_util
}  // namespace fetch
