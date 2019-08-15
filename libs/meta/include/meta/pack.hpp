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
//    type with member typedef type/static constexpr constant value, as appropriate.
// 2. Consequently, all template parameters are types only, value constants being recorded in
//    std::integral_constant.
//    Moreover, every returned value (not a type) is wrapped in a type with static member value,
//    and parameter functions, like F in Transform or Filter, are expected to act this way.
//    This can greatly facilitate the composition of templates actually.
// 3. For convenience, typedefs with names ending in -T and static constexpr constants with names
//    ending in -V are provided, following the same _t/_v pattern observed in STL.
//    These typedefs are used extensively to realize lazy evaluation results as needed.
// 4. List paradigm: all variadic evaluations in this chapter are done on Pack variadic template
//    only; this header's users only need to wrap actual parameter packs in that former and pass the
//    results as arguments here.
// 5. Idiomatically, this header follows C++ name conventions where possible; thus mapping function
//    is called Transform rather than Map, and Accumulate is used in place of Fold.
//    Unlike so, Head is still Head, for there's no C++ definition for its counterpart Tail,
//    thus its name is preserved to not break this entanglement.
//
// Below, `true type' designates a type that provides a public static constexpr member value that is
// true in boolean context.
// 'Size-valued' and 'bool-valued' denote a type that has a public static constexpr member value
// coercible to type std::size_t or bool, respectively.
// 'Wrapped T' stands for a struct with public member typedef type = T.

namespace fetch {
namespace pack {

/**
 * A simple function that wraps its arguments in a structure with a sole member typedef type.
 *
 * @param T an arbitrary type
 * @return structure S such that S::type is T
 */
// The default declaration accepts an arbitrary pack, so Type can be tail-called with arbitrary
// arg packs (known to be non-empty though).
template <class...>
struct Type;

template <class T>
using TypeT = typename Type<T>::type;  // technically, an identity mapping

template <class T, class... Ts>
struct Type<T, Ts...>
{
  using type = T;
};

// Two useful partial specializations of std::integral_constant.
/**
 * Returns an std::integral_constant of type std::size_t.
 *
 * @param i a compile-time size constant
 * @return std::integral_constant<std::size_t, i>
 */
template <std::size_t i>
using SizeConstant = std::integral_constant<std::size_t, i>;

template <std::size_t i>
static constexpr auto SizeConstantV = SizeConstant<i>::value;  // technically, an identity mapping

/**
 * Returns an std::integral_constant of type bool.
 *
 * @param b a compile-time boolean constant
 * @return std::integral_constant<bool, i>
 */
template <bool b>
using BoolConstant = std::integral_constant<bool, b>;

template <bool b>
static constexpr auto BoolConstantV = BoolConstant<b>::value;  // technically, an identity mapping

// Elementary unary operations on them, just in case
/**
 * Returns its argument plus one.
 *
 * @param N a class with static constant member value coercible to std::size_t
 * @return
 */
template <class N>
using Inc = std::integral_constant<std::decay_t<decltype(N::value)>, N::value + 1>;

template <class N>
static constexpr auto IncV = Inc<N>::value;

/**
 * Returns its argument minus one.
 *
 * @param N a class with static constant member value coercible to std::size_t
 * @return
 */
template <class N>
using Dec = std::integral_constant<std::decay_t<decltype(N::value)>, N::value - 1>;

template <class N>
static constexpr auto DecV = Dec<N>::value;

/**
 * Logical negation.
 *
 * @param N a class with static constant member value coercible to bool
 * @return
 */
template <class B>
using Not = BoolConstant<!B::value>;

template <class B>
static constexpr auto NotV = Not<B>::value;

/**
 * Asserted evaluations.
 * Several templates of this library end up in tail calls to other templates.
 * When unchecked, these tail calls can result in undefined instantiations (for invalid arguments)
 * but error messages are likely to appear cryptic in such cases, as the original template is
 * to be found in a rather incomprehensible "when instantiating an instantiation to be instantiated"
 * somewhere much much below. When<Condition, Impl> can be used in the most top-level call for
 * very simple guarded checks. Assert<condition, Impl> is an internal convenience function.
 */
/**
 * @param condition a constexpr value of type bool
 * @param T an arbitrary type
 * @return wrapped T when condition is true
 */
template<bool condition, class T> struct Assert;

template<bool condition, class T> using AssertT = typename Assert<condition, T>::type;

template<class T> struct Assert<true, T>: Type<T> {};

/**
 * @param Condition an arbitrary bool-valued type
 * @param T an arbitrary type
 * @return wrapped T when Condition is a true type
 */
template <class...> struct When;

template<class... Args> using WhenT = typename When<Args...>::type;

template<class Cond, class T> struct When<Cond, T>: Assert<Cond::value, T> {};

// The fundamental typelist definition.
template <class... Ts>
struct Pack
// It actually needs not to be defined, but a single empty pair of braces would do no harm here
// and can accidentally prove useful if one needs to quickly pass a pack of parameters to a
// function, so.
{
};

// Empty list.
using Nil = Pack<>;

// List of a single element.
template <class T>
using Singleton = Pack<T>;

/**
 * Basic list construction.
 *
 * @param Car an arbitrary type
 * @param Cdr an arbitrary Pack
 * @return Cdr with Car prepended
 */
template <class Car, class Cdr>
struct Cons;

template <class Car, class Cdr>
using ConsT = typename Cons<Car, Cdr>::type;

template <class Car, class... Cdr>
struct Cons<Car, Pack<Cdr...>> : Type<Pack<Car, Cdr...>>
{
};

/**
 * Detect if its argument has member type type.
 *
 * @param T an arbitrary type
 * @return a true type iff T is a class type with public member type type of kind *
 */
namespace detail_ {

struct Yes {};
struct No {};

template <class Arg>
constexpr decltype((std::declval<std::add_pointer_t<typename std::decay_t<Arg>::type>>(), Yes{}))
HasType(Arg &&) noexcept;

constexpr No HasType(...) noexcept;

}  // namespace detail_

template <class Arg> using HasMemberType = std::is_same<decltype(detail_::HasType(std::declval<Arg>())), detail_::Yes>;

template<class Arg> static constexpr auto HasMemberTypeV = HasMemberType<Arg>::value;

/**
 * Extract argument's member type type and wrap it back in a struct.
 *
 * @param T an arbitrary type
 * @return struct S such that S::type is T::type if the latter is a public type of kind *; empty
 * struct otherwise
 */
template <class Arg, bool = HasMemberTypeV<Arg>>
struct MemberType: Type<typename Arg::type>
{
};

template <class Arg, bool b = HasMemberTypeV<Arg>>
using MemberTypeT = typename MemberType<Arg, b>::type;

template <class Arg>
struct MemberType<Arg, false>
{
};

/**
 * Returns its argument if that has public member type type, otherwise wraps its argument in a
 * struct. If struct A { using type = int; }; struct B {}; then Flat<A> is A; Flat<B>::type is B.
 *
 * @param T an arbitrary type
 * @return
 */
template <class T>
using Flat = std::conditional<HasMemberTypeV<T>, T, Type<T>>;

template <class T>
using FlatT = typename Flat<T>::type;

/**
 * Leaves exactly one level of member type nesting.
 *     struct A {
 *         using type = struct {
 *             using type = struct {
 *                 using type = int;
 *             };
 *         };
 *     };
 *     Flatten<A>::type is int.
 *
 * @param T an arbitrary type
 * @return struct S such that S::type is T::type:: ... ::type and S::type has no member type called
 * type
 */
template <class T, bool = HasMemberTypeV<T>>
struct Flatten : Type<T>
{
};

template <class T, bool HasIt = HasMemberTypeV<T>>
using FlattenT = typename Flatten<T, HasIt>::type;

template <class T>
struct Flatten<T, true> : Flatten<typename T::type>
{
};

/**
 * Apply a function to a pack of arguments.
 *
 * @param F an arbitrary class template
 * @param Args an arbitrary Pack<T0...>
 * @return F<T0...>
 */
template <template <class...> class F, class Args>
struct Apply;

template <template <class...> class F, class Args>
using ApplyT = typename Apply<F, Args>::type;

template <template <class...> class F, class Args>
static constexpr auto ApplyV = ApplyT<F, Args>::value;

template <template <class...> class F, class... Args>
struct Apply<F, Pack<Args...>>: Type<F<Args...>> {};

/**
 * Function composition.
 * The first argument serves as a root in AST, all the rest args are its branches.
 * Note that the result should be used as Compose<F, Gs...>::template type. Consequently, ComposeT
 * is not defined.
 *
 * @param F the root of the AST
 * @param Gs... branches
 * @return lambda Args... . F(Gs(Args...)...)
 */
template <template <class...> class F, template <class...> class... Gs>
struct Compose
{
  template <class... Args>
  using type = F<Gs<Args...>...>;
};

// Basic list operations.
/**
 * Get the first element of a Pack.
 *
 * @param P a (non-empty) type Pack
 * @return P's first element
 */
template <class P>
struct Head;

template <class P>
using HeadT = typename Head<P>::type;

template <class Car, class... Cdr>
struct Head<Pack<Car, Cdr...>> : Type<Car>
{
};

/**
 * Iterate a pack one element to the right.
 *
 * @param P a (non-empty) type Pack
 * @return subpack comprised of all of P's elements but the first.
 */
template <class P>
struct Tail;

template <class P>
using TailT = typename Tail<P>::type;

template <class Car, class... Cdr>
struct Tail<Pack<Car, Cdr...>> : Type<Pack<Cdr...>>
{
};

/**
 * Map a function over a pack.
 * TODO(bipll): currently functions are not first-class objects here. This probably would need to be
 * fixed later.
 * (Where 'function' denotes a template class with type-only parameters,
 * and a 'first-class object' is merely a C++ type.)
 *
 * @param F an arbitrary class template
 * @param P an arbitrary Pack
 * @return
 */
template <template <class...> class F, class P>
struct Transform;

template <template <class...> class F, class P>
using TransformT = typename Transform<F, P>::type;

template <template <class...> class F, class... Ts>
struct Transform<F, Pack<Ts...>> : Type<Pack<F<Ts>...>>
{
};

/**
 * Filter a pack through a predicate.
 *
 * @param F a template class such that its instantiations have static constant value coercible to
 * bool
 * @param P an arbitrary Pack
 * @return Pack comprised of all elements Tn of P such that F<Pn> is a true type for each n
 */
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
struct Filter<F, Nil> : Type<Nil>
{
};

/**
 * Empty list predicate.
 *
 * @param P an arbitrary Pack
 * @return true type iff P is empty
 */
template <class P>
using Empty = std::is_same<P, Nil>;

template <class P>
static constexpr auto EmptyV = Empty<P>::value;

/**
 * List length.
 * Note that when defined as parameter packs, lists are always finite.
 *
 * @param P an arbitrary Pack
 * @return number of elements in P
 */
template <class P>
struct TupleSize;

template <class P>
static constexpr auto TupleSizeV = TupleSize<P>::value;

template <class... Ts>
struct TupleSize<Pack<Ts...>> : SizeConstant<sizeof...(Ts)>
{
};

// Simple sublists: take and drop.
/**
 * Retrieve first N elements of P.
 * If N is greater than the size of P, the result is P itself.
 *
 * @param N a size-valued class
 * @param P an arbitrary Pack
 * @return
 */
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
struct Take<N, Nil> : Type<Nil>
{
};

// take 0 _ == nil as well
template <class P>
struct Take<SizeConstant<0>, P> : Type<Nil>
{
};

// take 0 nil == nil, to disambiguate between the two above
template <>
struct Take<SizeConstant<0>, Nil> : Type<Nil>
{
};

/**
 * Skip first N elements of P.
 * If N is greater than the size of P, the result is empty.
 *
 * @param N a size-valued class
 * @param P an arbitrary Pack
 * @return
 */
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
struct Drop<N, Nil> : Type<Nil>
{
};

// drop 0 l == l
template <class P>
struct Drop<SizeConstant<0>, P> : Type<P>
{
};

// drop 0 nil == nil, to disambiguate between the two above
template <>
struct Drop<SizeConstant<0>, Nil> : Type<Nil>
{
};

// List halves, useful later.
/**
 * Left half of a Pack.
 * If argument's size is odd, left half is one element shorter than the right one.
 *
 * @param P an arbitrary Pack
 * @return
 */
template <class P>
using LeftHalf = Take<SizeConstant<TupleSizeV<P> / 2>, P>;

template <class P>
using LeftHalfT = typename LeftHalf<P>::type;

/**
 * Right half of a Pack.
 * If argument's size is odd, left half is one element shorter than the right one.
 *
 * @param P an arbitrary Pack
 * @return
 */
template <class P>
using RightHalf = Drop<SizeConstant<TupleSizeV<P> / 2>, P>;

template <class P>
using RightHalfT = typename RightHalf<P>::type;

/**
 * List element by index.
 *
 * @param Index a size-valued class
 * @param P an arbitrary Pack
 * @return P's element at Index
 */
template <class Index, class P>
struct TupleElement : AssertT<(TupleSizeV<P> > Index::value), TupleElement<SizeConstant<Index::value>, P>>
{
};

template <class Index, class P>
using TupleElementT = typename TupleElement<Index, P>::type;

template <std::size_t i, class P>
struct TupleElement<SizeConstant<i>, P>
  : TupleElement<SizeConstant<0>, DropT<SizeConstant<i>, P>>
{
};

template <class T, class... Ts>
struct TupleElement<SizeConstant<0>, Pack<T, Ts...>> : Type<T>
{
};

/**
 * Get pack's rightmost element, symmetric to Head.
 *
 * @param P a (non-empty) Pack
 * @return
 */
template <class P>
using Last = TupleElement<Dec<TupleSize<P>>, P>;

template <class P>
using LastT = typename Last<P>::type;

/**
 * Get a subpack comprised of all but the very last pack's elements.
 *
 * @param P a (non-empty) Pack
 * @return
 */
template <class P>
using Init = Take<Dec<TupleSize<P>>, P>;

template <class P>
using InitT = typename Init<P>::type;

}  // namespace pack
}  // namespace fetch

namespace std {

// Old clang a) defines std::tuple_size and std::tuple_element as classes and b) incorrectly issues
// a warning when a struct instantiates a class template.
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmismatched-tags"
#endif

template <class... Ts>
struct tuple_size<fetch::pack::Pack<Ts...>> : fetch::pack::TupleSize<fetch::pack::Pack<Ts...>>
{
};

template <size_t i, class... Ts>
struct tuple_element<i, fetch::pack::Pack<Ts...>>
  : fetch::pack::TupleElement<fetch::pack::SizeConstant<i>, fetch::pack::Pack<Ts...>>
{
};

#if defined(__clang__)
#pragma clang diagnostic pop
#endif

}  // namespace std

namespace fetch {
namespace pack {

/**
 * Left fold of a function over a pack.
 * Zero value is the leftmost element of the pack, so the latter should be non-empty.
 * (Similar to foldl1 of Prelude.PreludeList.)
 *
 * @param F an arbitrary class template
 * @param P a (non-empty) Pack<T0, T1..., Tn>
 * @return F<F< ... <F<T0, T1>, ... >, Tn>
 */
template <template <class...> class F, class Pack>
struct Accumulate;

template <template <class...> class F, class Pack>
using AccumulateT = typename Accumulate<F, Pack>::type;

template <template <class...> class F, class Pack>
static constexpr auto AccumulateV = AccumulateT<F, Pack>::value;

template <template <class...> class F, class Car, class Cadr, class... Cddr>
struct Accumulate<F, Pack<Car, Cadr, Cddr...>> : Accumulate<F, Pack<F<Car, Cadr>, Cddr...>>
{
};

template <template <class...> class F, class Last>
struct Accumulate<F, Pack<Last>> : Type<Last>
{
};

/**
 * Right fold of a function over a pack.
 * Zero value is the rightmost element of the pack, so the latter should be non-empty.
 * (Similar to foldr1 of Prelude.PreludeList.)
 *
 * @param F an arbitrary class template
 * @param P a (non-empty) Pack<T0, T1..., Tn>
 * @return F<T0, F< ... <F<Tn-1, Tn>> ... >>
 */
template <template <class...> class F, class Pack>
struct ReverseAccumulate;

template <template <class...> class F, class Pack>
using ReverseAccumulateT = typename ReverseAccumulate<F, Pack>::type;

template <template <class...> class F, class Pack>
static constexpr auto ReverseAccumulateV = ReverseAccumulateT<F, Pack>::value;

template <template <class...> class F, class Car, class Cadr, class... Cddr>
struct ReverseAccumulate<F, Pack<Car, Cadr, Cddr...>> : Type<F<Car, ReverseAccumulateT<F, Pack<Cadr, Cddr...>>>> {};

template <template <class...> class F, class Last>
struct ReverseAccumulate<F, Pack<Last>> : Type<Last>
{
};

/**
 * The most relaxed perl-style list constructor:
 * while Pack<T0...> only puts all the Tn's into a list,
 * Concat flattens its arguments splicing all the lists among them,
 * so if an argument is itself a Pack, its elements become elements of the result, but not the pack
 * itself:
 *     Pack<int, Pack<double, char>, std::string>
 *         is Pack<int, Pack<double, char>, std::string>;
 *
 *     ConcatT<int, Pack<double, char>, std::string>
 *         is Pack<int, double, char, std::string>.
 *
 * @param Packs... arbitrary types
 * @return a Pack of its arguments, with all Pack arguments spliced in
 */
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
struct Concat<Pack<LeftTs...>, Pack<RightTs...>> : Type<Pack<LeftTs..., RightTs...>>
{
};

template <>
struct Concat<> : Type<Nil>
{
};

/**
 * Partial function application.
 * Note that the result should be used as Bind<F, Args>::template type. Consequently, BindT is not
 * defined.
 *
 * @param F an arbitrary class template
 * @param Prefix a Pack<T0, ..., Tn>, n is less than F's maximum arity
 * @return struct S such that S::template type<Args...> is exactly F<T0, ..., Tn, Args...>
 */
template <template <class...> class F, class Prefix>
struct Bind;

template <template <class...> class F, class... Prefix>
struct Bind<F, Pack<Prefix...>>
{
  template <class... Args>
  using type = F<Prefix..., Args...>;
};

/**
 * A binary operator && on types.
 *
 * @param A a bool-valued type
 * @param B an arbitrary type
 * @return B if A is a true type; A otherwise
 */
template <class A, class B>
using And = std::conditional_t<bool(A::value), B, A>;

template <class A, class B>
static constexpr auto AndV = And<A, B>::value;

/**
 * A lisp-style operator && accepting an arbitrary amount of arguments and returning
 * the very last one of them if all others are true, or the leftmost false of them.
 * Conjunction of an empty pack is a true type.
 *
 * @param P an arbitrary Pack
 * @return the leftmost false element of P, or the last element, if none
 */
template <class P>
using Conjunction = Accumulate<And, ConsT<std::true_type, P>>;

template <class P>
using ConjunctionT = typename Conjunction<P>::type;

template <class P>
static constexpr auto ConjunctionV = ConjunctionT<P>::value;

/**
 * Check if every element of a pack satisfies a predicate.
 *
 * @param F a class template that evaluates to a bool-valued class on each element of the pack
 * (until one of them breaks the predicate, at least)
 * @param P an arbitrary Pack
 * @return
 */
template <template <class...> class F, class P>
using All = ConjunctionT<TransformT<F, P>>;

template <template <class...> class F, class P>
static constexpr auto AllV = All<F, P>::value;

/**
 * A binary operator || on types.
 *
 * @param A a bool-valued type
 * @param B an arbitrary type
 * @return A if A is a true type; B otherwise
 */
template <class A, class B>
using Or = std::conditional_t<bool(A::value), A, B>;

template <class A, class B>
static constexpr auto OrV = Or<A, B>::value;

/**
 * A lisp-style operator || accepting an arbitrary amount of arguments an returning the leftmost
 * true of them, or the very last one if all others are false. Disjunction of an empty pack is a
 * false type. Warning: DisjunctionT returns a member type type of the leftmost true type, not that
 * latter itself.
 *
 * @param P an arbitrary Pack
 * @return the leftmost true element of P, or the last one, if none
 */
template <class Pack>
using Disjunction = AccumulateT<Or, ConsT<std::false_type, Pack>>;

template <class Pack>
using DisjunctionT = typename Disjunction<Pack>::type;

template <class Pack>
static constexpr auto DisjunctionV = Disjunction<Pack>::value;

/**
 * Check if at least one element of a pack satisfies a predicate.
 *
 * @param F a class template that evaluates to a bool-valued class on each element of the pack
 * (until one of them satisfies the predicate, at least)
 * @param P an arbitrary Pack
 * @return
 */
template <template <class...> class F, class Pack>
using Any = Disjunction<TransformT<F, Pack>>;

template <template <class...> class F, class Pack>
static constexpr auto AnyV = Any<F, Pack>::value;

/**
 * Check if type is found in a pack.
 *
 * @param T an arbitrary type
 * @param P an arbitrary Pack
 * @return true type iff there's an element in P equal to T, in the sense of std::is_same
 */
template <class T, class P>
using IsAnyOf = Disjunction<TransformT<Bind<std::is_same, Pack<T>>::template type, P>>;

template <class T, class P>
static constexpr auto IsAnyOfV = IsAnyOf<T, P>::value;

// IsInvocable and InvokeResult are limited backports of their C++17's
// namesakes. For the purpose of this header, argument packs are encapsulated in Packs.
// IsNothrowInvocable can hardly be reasonably portably implemented in C++14.
namespace detail_ {

template <class F, class... Args>
constexpr decltype((std::declval<F>()(std::declval<Args>()...), Yes{})) Invocable(
    F &&, Args &&...) noexcept;
constexpr No Invocable(...) noexcept;

}  // namespace detail_

/**
 * Check if a function (here, the real C++ function) can be called with arguments of such types.
 *
 * @param F a function type
 * @param P a Pack<T0...> of argument types
 * @return true type if f(t0...) is a valid expression, where f is of type F and tn is of type Tn,
 * for each n
 */
template <class F, class P>
struct IsInvocable;

template <class F, class P>
static constexpr auto IsInvocableV = IsInvocable<F, P>::value;

template <class F, class... Args>
struct IsInvocable<F, Pack<Args...>>
  : std::is_same<decltype(detail_::Invocable(std::declval<F>(), std::declval<Args>()...)), detail_::Yes>
{
};

/**
 * Get the result type of an invocation of a function (here, the real C++ function) over arguments
 * of such types. Warning: IsInvocable<F, P> should be a true type.
 *
 * @param F a function type
 * @param P a Pack<T0...> of argument types
 * @return struct S such that f(t0...) is of type S::type, where f is of type F and tn is of type
 * Tn, for each n
 */
template <class F, class P>
struct InvokeResult;

template <class F, class P>
using InvokeResultT = typename InvokeResult<F, P>::type;

template <class F, class... Args>
struct InvokeResult<F, Pack<Args...>>
  : Type<decltype(std::declval<F>()(std::declval<Args>()...))>
{
};

/**
 * Top-down linear switch.
 * Arguments are split in coupled pairs: Condition, Then-Expression.
 * The result has member typedef type equal to the leftmost Then-Expression
 * whose preceeding Condition is a true type. If none of the Conditions is true then
 * the number of elements in Clauses should be odd, and its last element is taken as the default
 * statement.
 *     CaseT<Pack<
 *         std::false_type, int,
 *         SizeConstant<0>, double,
 *         std::true_type, std::string,
 *         char>>
 *     is
 *         std::string;
 *
 *     CaseT<Pack<
 *         std::false_type, int,
 *         SizeConstant<0>, double,
 *         char>>
 *     is
 *         char.
 * If all the Conditions are false and there's no Default statement, Case::type is void.
 *
 * @param Clauses a Pack
 * @return
 */
template <class Clauses>
struct Case;

template <class Clauses>
using CaseT = typename Case<Clauses>::type;

template <class If, class Then, class... Else>
struct Case<Pack<If, Then, Else...>> : std::conditional<bool(If::value), Then, CaseT<Pack<Else...>>>
{
};

template <class Default>
struct Case<Pack<Default>> : Type<Default>
{
};

template <>
struct Case<Nil>
{
  using type = void;
};

/**
 * Select the leftmost member type type defined in pack elements.
 *
 *     struct A { using type = int; };
 *
 *     Select<char, double, A, std::string>
 *         is int.
 *
 * @param Clauses an arbitrary Pack
 * @return
 */
template <class Clauses>
struct Select;

template <class Clauses>
using SelectT = typename Select<Clauses>::type;

template <class Car, class... Cdr>
struct Select<Pack<Car, Cdr...>>
  : std::conditional_t<HasMemberTypeV<Car>, Car, Select<Pack<Cdr...>>>
{
};

// Sort pack by member value in ascending order, leaving only unique (non-mutually equal) keys.
// Comparison predicate.
/**
 * Check if A is less-valued than B.
 * A::value and B::value should be <-comparable.
 *
 * @param A a class type with static constexpr member value
 * @param B a class type with static constexpr member value
 * @return
 */
template <class A, class B>
struct LessThan : BoolConstant<(A::value < B::value)>
{
};

template <class A, class B>
static constexpr auto LessThanV = LessThan<A, B>::value;

// We'll use mergesort, to make it in guaranteed logarithmic compile-time.
/**
 * Merge the two sorted packs.
 * All the elements in result are unique, in the sense that,
 * for every two elements A and B of the result, either A::value < B::value, or in reverse.
 *
 * @param Left a Pack
 * @param Right a Pack
 * @return
 */
template <class Left, class Right>
struct UniqueMerge;

template <class Left, class Right>
using UniqueMergeT = typename UniqueMerge<Left, Right>::type;

template <class Left, class Right>
struct UniqueMerge
  : Case<Pack<
        LessThan<HeadT<Left>, HeadT<Right>>, ConsT<HeadT<Left>, UniqueMergeT<TailT<Left>, Right>>,
        LessThan<HeadT<Right>, HeadT<Left>>, ConsT<HeadT<Right>, UniqueMergeT<Left, TailT<Right>>>,
        ConsT<HeadT<Left>, UniqueMergeT<TailT<Left>, TailT<Right>>>>>
{
};

template <class Left>
struct UniqueMerge<Left, Nil> : Type<Left>
{
};

template <class Right>
struct UniqueMerge<Nil, Right> : Type<Right>
{
};

template <>
struct UniqueMerge<Nil, Nil> : Type<Nil>
{
};

/**
 * Sort a pack.
 *
 * @param P a Pack (of numerically-valued types)
 */
template <class P>
struct UniqueSort;

template <class P>
using UniqueSortT = typename UniqueSort<P>::type;

template <class P>
struct UniqueSort : UniqueMerge<UniqueSortT<LeftHalfT<P>>, UniqueSortT<RightHalfT<P>>>
{
};

template <class T>
struct UniqueSort<Pack<T>> : Type<Pack<T>>
{
};

template <>
struct UniqueSort<Nil> : Type<Nil>
{
};

// A companion predicate.
/**
 * Returns true type if P is uniquely sorted in ascending order.
 *
 * @param P a Pack
 * @return
 */
template <class P>
struct IsUniquelySorted;

template <class P>
static constexpr auto IsUniquelySortedV = IsUniquelySorted<P>::value;

template <class Car, class Cadr, class... Cddr>
struct IsUniquelySorted<Pack<Car, Cadr, Cddr...>>
  : And<LessThan<Car, Cadr>, IsUniquelySorted<Pack<Cadr, Cddr...>>>
{
};

template <class Car>
struct IsUniquelySorted<Pack<Car>> : std::true_type
{
};

template <>
struct IsUniquelySorted<Nil> : std::true_type
{
};

/**
 * Remove duplicates, in the sense of std::is_same, from a pack without changing the order of unique
 * elements.
 *
 * Original taken from A. Alexandrescu's Modern C++ Design, and converted into this library wording.
 *
 * @param P an arbitrary Pack
 * @return
 */
template <class P>
struct MakeUnique;

template <class P>
using MakeUniqueT = typename MakeUnique<P>::type;

template <class P>
struct MakeUnique
  : Cons<HeadT<P>, Filter<Compose<Not, Bind<std::is_same, HeadT<P>>::template type>::template type,
                          MakeUniqueT<TailT<P>>>>
{
};

template <class Car, class... Cddr>
struct MakeUnique<Pack<Car, Car, Cddr...>> : MakeUnique<Pack<Car, Cddr...>>
{
};

template <>
struct MakeUnique<Nil> : Type<Nil>
{
};

/**
 * Retrieve arguments of a template instantiation, or a function, packed in a Pack.
 * For member functions, implicit first argument (i.e. the class this function is a member of)
 * is not included in the result.
 *
 * @param T either a template class instantiation, or a function type
 * @return
 */
template <class T>
struct Args;

template <class T>
using ArgsT = typename Args<T>::type;

template <template <class...> class Ctor, class... As>
struct Args<Ctor<As...>> : Type<Pack<As...>>
{
};

template <class R, class... As>
struct Args<R (*)(As...)> : Type<Pack<As...>>
{
};

template <class R, class C, class... As>
struct Args<R (C::*)(As...)> : Type<Pack<As...>>
{
};

template <class R, class C, class... As>
struct Args<R (C::*)(As...) const> : Type<Pack<As...>>
{
};

template <class R, class C, class... As>
struct Args<R (C::*)(As...) &> : Type<Pack<As...>>
{
};

template <class R, class C, class... As>
struct Args<R (C::*)(As...) const &> : Type<Pack<As...>>
{
};

template <class R, class C, class... As>
struct Args<R (C::*)(As...) &&> : Type<Pack<As...>>
{
};

template <class R, class C, class... As>
struct Args<R (C::*)(As...) const &&> : Type<Pack<As...>>
{
};

}  // namespace pack
}  // namespace fetch
