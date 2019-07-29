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
// Or, say, Casees can be written as
//
//     Case<pack::Pack<If1, Then1>, pack::Pack<If2, Then2>, pack::Pack<If3, Then3>>
//
// for greater readability.

// TypeConstant<T> is an otherwise empty structure whose sole member is a typedef type = T.
template <class T>
using TypeConstant = pack::Constant<T>;
template <class T>
using TypeConstantT = typename TypeConstant<T>::type;  // technically, an identity mapping

// Two useful partial specializations of std::integral_constant.
template <std::size_t i>
using SizeConstant = pack::SizeConstant<i>;

template <bool b>
using BoolConstant = pack::BoolConstant<b>;

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

// Case<Clauses...> implements top-down linear switch.
// Its arguments are split in coupled pairs: Condition, Then-Expression.
// Its member typedef type is equal to the leftmost Then-Expression
// whose preceeding Condition is a true type. If none of the Conditions is true then
// the number of elements in Clauses should be odd, and its last element is taken as the default
// statement.
//     CaseT<std::false_type, int, SizeConstant<0>, double, std::true_type, std::string, char> is
//     std::string; CaseT<std::false_type, int, SizeConstant<0>, double, char> is char.
// If all the Conditions are false and there's no Default statement, Case::type is void.
template <class... Clauses>
using Case = pack::Case<pack::ConcatT<Clauses...>>;

template <class... Clauses>
using CaseT = typename Case<Clauses...>::type;

// CopyReferenceKind<A, B> provide a member typedef type that is B with ref-qualifier changed to
// that of A.
template <class Source, class Dest>
using CopyReferenceKind = Case<std::is_lvalue_reference<Source>, std::add_lvalue_reference_t<Dest>,
                               std::is_rvalue_reference<Source>,
                               std::add_rvalue_reference_t<std::decay_t<Dest>>, std::decay_t<Dest>>;

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
