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

#include <tuple>
#include <type_traits>
#include <utility>

// Simple composability rules.
// 1. Evaluation is done in a lazy-like manner: the result of a computation is encapsulated in a
// type
//    with member typedef type/static constexpr constant value, as appropriate.
// 2. Consequently, all template parameters are types only, value constants being recorded in
// std::integral_constant.
//    Moreover, every returned value (not a type) is wrapped in a type with static member value,
//    and parameter functions, like F in Transform or Filter, are expected to act this way.
//    This can greatly facilitate the composition of templates actually.
// 3. For convenience, typedefs with names ending in -T and static constexpr constants with names
// ending in -V
//    are provided, following the same _t/_v pattern observed in STL.
//    These typedefs are used extensively to realize lazy evaluation results as needed.
// 4. List paradigm: all variadic evaluations in this chapter are done on Pack variadic template
// only;
//    this header's users only need to wrap actual parameter packs in that former and pass the
//    results as arguments here.
// 5. Idiomatically, this header follows C++ name conventions where possible; thus mapping function
//    is called Transform rather than Map, and Accumulate is used in place of Fold.
//    Unlike so, Head is still Head, for there's no C++ definition for its counterpart Tail,
//    thus its name is preserved to not break this entanglement.
//
// Below, `true type' designates a type that provides a public static constexpr member value that is
// true in boolean context.

namespace fetch {
namespace pack {

// Constant<T> is an otherwise empty structure whose sole member is a typedef type = T.
template <class...>
struct Constant;
template <class T>
using ConstantT = typename Constant<T>::type;  // technically, an identity mapping

template <class T, class... Ts>
struct Constant<T, Ts...>
{
  using type = T;
};

namespace detail_ {

struct Yes
{
  char a, b;
};

template <class Arg>
constexpr decltype((std::declval<std::add_pointer_t<typename std::decay_t<Arg>::type>>(), Yes{}))
HasType(Arg &&) noexcept;

constexpr char HasType(...) noexcept;

}  // namespace detail_

// HasMemberTypeV<T> is true if T::type is a public type of kind *, and false otherwise.
template <class Arg>
static constexpr bool HasMemberTypeV = sizeof(detail_::HasType(std::declval<Arg>())) ==
                                       sizeof(detail_::Yes);

// MemberType<T> is an otherwise empty structure whose sole member is a typedef type = T::type.
template <class Arg, bool = HasMemberTypeV<Arg>>
struct MemberType
{
  using type = typename Arg::type;
};
template <class Arg, bool b = HasMemberTypeV<Arg>>
using MemberTypeT = typename MemberType<Arg, b>::type;

template <class Arg>
struct MemberType<Arg, false>
{
};

// Flat<T> keeps member ::types of nesting deeper:
// If
//    struct A { using type = int; };
//    sturct B {};
// then
//    Flat<A> is A;
//    Flat<B>::type is B.
template <class T>
using Flat = std::conditional<HasMemberTypeV<T>, T, Constant<T>>;
template <class T>
using FlatT = typename Flat<T>::type;

// Flatten<T> leaves exactly one level of member type nesting.
//     struct A { using type = struct { using type = struct { using type = int; }; }; };
//     Flatten<A>::type is int.
template <class T, bool = HasMemberTypeV<T>>
struct Flatten : Constant<T>
{
};
template <class T, bool HasIt = HasMemberTypeV<T>>
using FlattenT = typename Flatten<T, HasIt>::type;

template <class T>
struct Flatten<T, true> : Flatten<typename T::type>
{
};

// Two useful partial specializations of std::integral_constant.
template <std::size_t i>
using SizeConstant = std::integral_constant<std::size_t, i>;
template <std::size_t i>
static constexpr auto SizeConstantV = SizeConstant<i>::value;  // technically, an identity mapping

template <bool b>
using BoolConstant = std::integral_constant<bool, b>;
template <bool b>
static constexpr auto BoolConstantV = BoolConstant<b>::value;  // technically, an identity mapping

// Elementary unary operations on them, just in case
template <class N>
using Inc = SizeConstant<N::value + 1>;
template <class N>
static constexpr auto IncV = Inc<N>::value;

template <class N>
using Dec = SizeConstant<N::value - 1>;
template <class N>
static constexpr auto DecV = Dec<N>::value;

template <class B>
using Not = BoolConstant<!B::value>;
template <class B>
static constexpr auto NotV = Not<B>::value;

// The fundamental typelist definition.
template <class... Ts>
struct Pack
// It actually needs not definition, but a single empty pair of braces would do no harm here
// and can accidentally prove useful if one needs to quickly pass a pack of parameters to a
// function, so.
{
};

using Nil = Pack<>;

template <class T>
using Singleton = Pack<T>;

// Basic list construction.
template <class Car, class Cdr>
struct Cons;
template <class Car, class Cdr>
using ConsT = typename Cons<Car, Cdr>::type;
template <class Car, class... Cdr>
struct Cons<Car, Pack<Cdr...>> : Constant<Pack<Car, Cdr...>>
{
};

// Head<Pack> evaluates to Pack's first element.
// It is an error if Head is applied to Nil.
template <class P>
struct Head;

template <class P>
using HeadT = typename Head<P>::type;

template <class Car, class... Cdr>
struct Head<Pack<Car, Cdr...>> : Constant<Car>
{
};

// Tail<P> evaluates to a Pack comprised of all P's elements but the first.
// It is an error if Tail is applied to Nil.
template <class P>
struct Tail;

template <class P>
using TailT = typename Tail<P>::type;

template <class Car, class... Cdr>
struct Tail<Pack<Car, Cdr...>> : Constant<Pack<Cdr...>>
{
};

// Map a function over a list.
// TODO(bipll): currently functions are not first-class objects here. This probably would need to be
// fixed later. (Where 'function' denotes a template class with type-only parameters, and a
// 'first-class object' is merely a C++ type.)
template <template <class...> class F, class P>
struct Transform;

template <template <class...> class F, class P>
using TransformT = typename Transform<F, P>::type;

template <template <class...> class F, class... Ts>
struct Transform<F, Pack<Ts...>> : Constant<Pack<F<Ts>...>>
{
};

// Filter list through a predicate.
template <template <class...> class F, class P>
struct Filter;

template <template <class...> class F, class P>
using FilterT = typename Filter<F, P>::type;

template <template <class...> class F, class Car, class... Cdr>
struct Filter<F, Pack<Car, Cdr...>>
  : std::conditional_t<bool(F<Car>::value), Cons<Car, FilterT<F, Pack<Cdr...>>>,
                       Filter<F, Pack<Cdr...>>>
{
};

template <template <class...> class F>
struct Filter<F, Nil> : Constant<Nil>
{
};

// Empty list predicate.
template <class P>
using Empty = std::is_same<P, Nil>;

template <class P>
static constexpr auto EmptyV = Empty<P>::value;

// List length.
// Note that when defined as parameter packs, lists are always finite.
template <class P>
struct TupleSize;

template <class P>
static constexpr auto TupleSizeV = TupleSize<P>::value;

template <class... Ts>
struct TupleSize<Pack<Ts...>> : SizeConstant<sizeof...(Ts)>
{
};

// Simple sublists: take and drop.
template <class N, class P>
struct Take;

template <class N, class P>
using TakeT = typename Take<N, P>::type;
// (take n l) is l's prefix of length n (or l itself, if its length is not greater than n).
template <class N, class P>
struct Take : Cons<HeadT<P>, TakeT<Dec<N>, TailT<P>>>
{
};
// take _ nil == nil
template <class N>
struct Take<N, Nil> : Constant<Nil>
{
};
// take 0 _ == nil as well
template <class P>
struct Take<SizeConstant<0>, P> : Constant<Nil>
{
};
// take 0 nil == nil, to disambiguate between the two above
template <>
struct Take<SizeConstant<0>, Nil> : Constant<Nil>
{
};

template <class N, class P>
struct Drop;

template <class N, class P>
using DropT = typename Drop<N, P>::type;
// (drop n l) is l's suffix past the first n elements (or an empty list, if l's length is not
// greater than n).
template <class N, class P>
struct Drop : Drop<Dec<N>, TailT<P>>
{
};
// drop _ nil == nil
template <class N>
struct Drop<N, Nil> : Constant<Nil>
{
};
// drop 0 l == l
template <class P>
struct Drop<SizeConstant<0>, P> : Constant<P>
{
};
// drop 0 nil == nil, to disambiguate between the two above
template <>
struct Drop<SizeConstant<0>, Nil> : Constant<Nil>
{
};

// List halves, useful later.
template <class P>
using LeftHalf = Take<SizeConstant<TupleSizeV<P> / 2>, P>;
template <class P>
using LeftHalfT = typename LeftHalf<P>::type;

template <class P>
using RightHalf = Drop<SizeConstant<TupleSizeV<P> / 2>, P>;
template <class P>
using RightHalfT = typename RightHalf<P>::type;

// List element by index.
template <class Index, class P>
struct TupleElement : TupleElement<SizeConstant<Index::value>, P>
{
};
template <class Index, class P>
using TupleElementT = typename TupleElement<Index, P>::type;

// Implemented by dividing index by two.
// This way compiler only has to instantiate logarithmic amount of templates per a list element.
// Say, for a 1000000th list element naive linear search would require one million instantions,
// while binary split would only need 20 or so.
// This is basically a simple case of dynamic programming on types.
template <std::size_t i, class P>
struct TupleElement<SizeConstant<i>, P>
  : TupleElement<SizeConstant<i / 2>, DropT<SizeConstant<i - i / 2>, P>>
{
};

template <class T, class... Ts>
struct TupleElement<SizeConstant<0>, Pack<T, Ts...>> : Constant<T>
{
};

// Last<Pack> evaluates to Pack's last element.
// It is an error if Last is applied to Nil.
template <class P>
using Last = TupleElement<Dec<TupleSize<P>>, P>;
template <class P>
using LastT = typename Last<P>::type;

// Init<Pack> evaluates to a Pack comprised of all but the very last Pack's elements.
// It is an error if Init is applied to Nil.
template <class P>
using Init = Take<Dec<TupleSize<P>>, P>;
template <class P>
using InitT = typename Init<P>::type;

}  // namespace pack
}  // namespace fetch

namespace std {

template <class... Ts>
struct tuple_size<fetch::pack::Pack<Ts...>> : fetch::pack::TupleSize<fetch::pack::Pack<Ts...>>
{
};
template <size_t i, class... Ts>
struct tuple_element<i, fetch::pack::Pack<Ts...>>
  : fetch::pack::TupleElement<fetch::pack::SizeConstant<i>, fetch::pack::Pack<Ts...>>
{
};

}  // namespace std

namespace fetch {
namespace pack {

// Accumulate<F, Pack> is a left fold of F over Pack.
// Pack should be non-empty.
template <template <class...> class F, class Pack>
struct Accumulate;
template <template <class...> class F, class Pack>
using AccumulateT = typename Accumulate<F, Pack>::type;
template <template <class...> class F, class Pack>
static constexpr auto AccumulateV = Accumulate<F, Pack>::value;

template <template <class...> class F, class Car, class Cadr, class... Cddr>
struct Accumulate<F, Pack<Car, Cadr, Cddr...>> : Accumulate<F, Pack<F<Car, Cadr>, Cddr...>>
{
};

template <template <class...> class F, class Last>
struct Accumulate<F, Pack<Last>> : Constant<Last>
{
};

// Concat is the most relaxed perl-style list constructor:
// while Pack<T0...> only puts all the Tn's into a list,
// Concat flattens its arguments splicing all the lists among them,
// so if an argument is itself a Pack, its elements become elements of the result, not the pack
// itself:
//     Pack<int, Pack<double, char>, std::string> is Pack<int, Pack<double, char>, std::string>;
//     ConcatT<int, Pack<double, char>, std::string> is Pack<int, double, char, std::string>.
template <class... Packs>
struct Concat;
template <class... Packs>
using ConcatT = typename Concat<Packs...>::type;

template <class T, class... Packs>
struct Concat<T, Packs...> : Cons<T, ConcatT<Packs...>>
{
};

template <class... Ts, class... Packs>
struct Concat<Pack<Ts...>, Packs...> : Concat<Pack<Ts...>, ConcatT<Packs...>>
{
};

template <class... LeftTs, class... RightTs>
struct Concat<Pack<LeftTs...>, Pack<RightTs...>> : Constant<Pack<LeftTs..., RightTs...>>
{
};

template <>
struct Concat<> : Constant<Nil>
{
};

// Bind<F, Args...> is an otherwise empty structure whose sole member is a template type such that
// Bind<F, T...>::template type<U...> is the same as F<T..., U...>.
// Bind is the only type-containing struct defined in this header that does not have accompanying
// BindT, current C++ wouldn't allow that.
template <template <class...> class F, class Prefix>
struct Bind;

template <template <class...> class F, class... Prefix>
struct Bind<F, Pack<Prefix...>>
{
  template <class... Args>
  using type = F<Prefix..., Args...>;
};

// And<A, B> is a lispish binary operator && on types.
// Its result is a true type if both A and B are.
// A::value should be defined and convertible to bool.
// No requirements imposed on B.
template <class A, class B>
using And = std::conditional_t<bool(A::value), B, A>;
template <class A, class B>
static constexpr auto AndV = And<A, B>::value;

// Conjunction<Pack> is a true type iff every element of Pack is.
template <class Pack>
using Conjunction = AccumulateT<And, ConsT<std::true_type, Pack>>;
template <class Pack>
using ConjunctionT = typename Conjunction<Pack>::type;
template <class Pack>
static constexpr auto ConjunctionV = Conjunction<Pack>::value;

// All<F, Pack> is a true type iff F applied to each element of Pack returns a true type.
template <template <class...> class F, class Pack>
using All = Conjunction<TransformT<F, Pack>>;
template <template <class...> class F, class Pack>
static constexpr auto AllV = All<F, Pack>::value;

// Or<A, B> is a lispish binary operator || on types.
// Its result is a true type if either A or B is.
// A::value should be defined and convertible to bool.
// No requirements imposed on B.
template <class A, class B>
using Or = std::conditional_t<bool(A::value), A, B>;
template <class A, class B>
static constexpr auto OrV = Or<A, B>::value;

// Disjunction<Pack> is a true type iff at least one element of Pack is.
template <class Pack>
using Disjunction = AccumulateT<Or, ConsT<std::false_type, Pack>>;
template <class Pack>
using DisjunctionT = typename Disjunction<Pack>::type;
template <class Pack>
static constexpr auto DisjunctionV = Disjunction<Pack>::value;

// Any<F, Pack> is a true type iff F applied to at least one element of Pack returns a true type.
template <template <class...> class F, class Pack>
using Any = Disjunction<TransformT<F, Pack>>;
template <template <class...> class F, class Pack>
static constexpr auto AnyV = Any<F, Pack>::value;

// IsAnyOf<T, Pack> is a true type if T is equal, in the sense of std::is_same, to at least one
// element of Pack.
template <class T, class P>
using IsAnyOf = Disjunction<TransformT<Bind<std::is_same, Singleton<T>>::template type, P>>;
template <class T, class P>
static constexpr auto IsAnyOfV = IsAnyOf<T, P>::value;

namespace detail_ {

template <class F, class... Args>
constexpr decltype((std::declval<F>()(std::declval<Args>()...), Yes{})) Invocable(
    F &&, Args &&...) noexcept;
constexpr char Invocable(...) noexcept;

}  // namespace detail_

// IsInvocable and InvokeResult are limited backports of their C++17's
// namesakes. For the purpose of this header, argument packs are encapsulated in Packs.
// IsNothrowInvocable can hardly be reasonably portably implemented in C++14.
template <class F, class P>
struct IsInvocable;
template <class F, class P>
static constexpr auto IsInvocableV = IsInvocable<F, P>::value;
template <class F, class... Args>
struct IsInvocable<F, Pack<Args...>>
  : BoolConstant<sizeof(detail_::Invocable(std::declval<F>(), std::declval<Args>()...)) ==
                 sizeof(detail_::Yes)>
{
};

template <class F, class P>
struct InvokeResult;
template <class F, class P>
using InvokeResultT = typename InvokeResult<F, P>::type;
template <class F, class... Args>
struct InvokeResult<F, Pack<Args...>>
  : Constant<decltype(std::declval<F>()(std::declval<Args>()...))>
{
};

// Switch<Clauses> implements top-down linear switch.
// Its arguments are split in coupled pairs: Condition, Then-Expression.
// Its member typedef type is equal to the leftmost Then-Expression
// whose preceeding Condition is a true type. If none of the Conditions is true then
// the number of elements in Clauses should be odd, and its last element is taken as the default
// statement.
//     SwitchT<std::false_type, int, SizeConstant<0>, double, std::true_type, std::string, char> is
//     std::string; SwitchT<std::false_type, int, SizeConstant<0>, double, char> is char.
// If all the Conditions are false and there's no Default statement, Switch::type is void.
template <class Clauses>
struct Switch;
template <class Clauses>
using SwitchT = typename Switch<Clauses>::type;

template <class If, class Then, class... Else>
struct Switch<Pack<If, Then, Else...>>
  : std::conditional<bool(If::value), Then, SwitchT<Pack<Else...>>>
{
};

template <class Default>
struct Switch<Pack<Default>> : Constant<Default>
{
};

template <>
struct Switch<Nil>
{
  using type = void;
};

// Select<Pack<Type1, Type2...>> provides member typedef type which is the leftmost Typen::type
// available. It is an error if none of template parameters has member type named type.
template <class Clauses>
struct Select;
template <class Clauses>
using SelectT = typename Select<Clauses>::type;

template <class Car, class... Cdr>
struct Select<Pack<Car, Cdr...>>
  : std::conditional_t<HasMemberTypeV<Car>, Car, Select<Pack<Cdr...>>>
{
};

// Sort pack by member value in increasing order, leaving only unique (non-mutually equal) keys.
// Comparison predicate.
template <class A, class B>
using LessThan = BoolConstant<(A::value < B::value)>;
template <class A, class B>
static constexpr auto LessThanV = LessThan<A, B>::value;

// We'll use mergesort, to make it in guaranteed logarithmic compile-time.
// If two keys compare equal, the leftmost is retained.
template <class Left, class Right>
struct UniqueMerge;
template <class Left, class Right>
using UniqueMergeT = typename UniqueMerge<Left, Right>::type;
template <class Left, class Right>
struct UniqueMerge
  : Switch<Pack<
        LessThan<HeadT<Left>, HeadT<Right>>, ConsT<HeadT<Left>, UniqueMergeT<TailT<Left>, Right>>,
        LessThan<HeadT<Right>, HeadT<Left>>, ConsT<HeadT<Right>, UniqueMergeT<Left, TailT<Right>>>,
        ConsT<HeadT<Left>, UniqueMergeT<TailT<Left>, TailT<Right>>>>>
{
};
template <class Left>
struct UniqueMerge<Left, Nil> : Constant<Left>
{
};
template <class Right>
struct UniqueMerge<Nil, Right> : Constant<Right>
{
};
template <>
struct UniqueMerge<Nil, Nil> : Constant<Nil>
{
};

template <class P>
struct UniqueSort;
template <class P>
using UniqueSortT = typename UniqueSort<P>::type;
template <class P>
struct UniqueSort : UniqueMerge<UniqueSortT<LeftHalfT<P>>, UniqueSortT<RightHalfT<P>>>
{
};
template <class T>
struct UniqueSort<Pack<T>> : Constant<Pack<T>>
{
};
template <>
struct UniqueSort<Nil> : Constant<Nil>
{
};

// A companion predicate.
template <class P>
struct IsSorted;
template <class P>
static constexpr auto IsSortedV = IsSorted<P>::value;

template <class Car, class Cadr, class... Cddr>
struct IsSorted<Pack<Car, Cadr, Cddr...>> : And<LessThan<Car, Cadr>, IsSorted<Pack<Cadr, Cddr...>>>
{
};
template <class Car>
struct IsSorted<Pack<Car>> : std::true_type
{
};
template <>
struct IsSorted<Nil> : std::true_type
{
};

// Args retrieves arguments of a template instantiation, or a function, and packs them in a Pack.
template <class T>
struct Args;
template <class T>
using ArgsT = typename Args<T>::type;

template <template <class...> class Ctor, class... As>
struct Args<Ctor<As...>> : Constant<Pack<As...>>
{
};

template <class R, class... As>
struct Args<R (*)(As...)> : Constant<Pack<As...>>
{
};

template <class R, class C, class... As>
struct Args<R (C::*)(As...)> : Constant<Pack<As...>>
{
};

template <class R, class C, class... As>
struct Args<R (C::*)(As...) const> : Constant<Pack<As...>>
{
};

template <class R, class C, class... As>
struct Args<R (C::*)(As...) &> : Constant<Pack<As...>>
{
};

template <class R, class C, class... As>
struct Args<R (C::*)(As...) const &> : Constant<Pack<As...>>
{
};

template <class R, class C, class... As>
struct Args<R (C::*)(As...) &&> : Constant<Pack<As...>>
{
};

template <class R, class C, class... As>
struct Args<R (C::*)(As...) const &&> : Constant<Pack<As...>>
{
};

}  // namespace pack
}  // namespace fetch
