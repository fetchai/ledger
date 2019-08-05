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

#include "meta/type_traits.hpp"
#include "meta/type_util.hpp"

#include <functional>
#include <type_traits>
#include <utility>

namespace fetch {
namespace value_util {

/**
 * No-op as it is.
 * When invoked on any arguments it does nothing at all.
 * One of its possible uses, it can simplify/facilitate writing noexcept specifications:
 * noexcept(no_op(a, b...)) should be the same as noexcept(a) && noecxept(b) && ...
 *
 * @param _ an arbitrary amount of ignored arguments of any types.
 */
template <class... Ts>
constexpr void NoOp(Ts &&...)
{}

/**
 * Accumulate(f, a0, a1, a2, ..., an) returns f(f(...(f(a0, a1), a2), ...), an).
 * It is a left-fold, similar to std::accumulate, but operating on argument packs rather than
 * ranges.
 */

// The zero case: the pack is empty past a0.
template <class F, typename RV>
constexpr auto Accumulate(F &&, RV &&rv) noexcept(std::is_nothrow_move_constructible<RV>::value)
{
  return std::forward<RV>(rv);
}

// The recursion base: last step, only two values left.
template <class F, typename A, typename B>
constexpr auto Accumulate(F &&f, A &&a, B &&b)
{
  return std::forward<F>(f)(std::forward<A>(a), std::forward<B>(b));
}

// The generic case.
template <class F, typename A, typename B, typename... Seq>
constexpr auto Accumulate(F &&f, A &&a, B &&b, Seq &&... seq);

template <class F, typename A, typename B, typename... Seq>
constexpr auto Accumulate(F &&f, A &&a, B &&b, Seq &&... seq)
{
  return Accumulate(std::forward<F>(f), f(std::forward<A>(a), std::forward<B>(b)),
                    std::forward<Seq>(seq)...);
}

// Simple uses of Accumulate().
// Since Accumulate() itself operates on packs rather than ranges,
// there's no special overload to calculate simple sum, like in STL.

/*
 * Sum computes arithmetic sum (in the sense of operator+, so std::strings are allowed, too) of its
 * arguments.
 *
 * @param h the very first summand, distinct from the rest of the pack mostly to infer return type.
 * @param ts... al the rest summands
 * @return h + ts...
 */
template <typename H, typename... Ts>
constexpr auto Sum(H &&h,
                   Ts &&... ts) noexcept(noexcept(Accumulate(std::plus<void>{}, std::forward<H>(h),
                                                             std::forward<Ts>(ts)...)))
{
  return Accumulate(std::plus<void>{}, std::forward<H>(h), std::forward<Ts>(ts)...);
}

/*
 * Product computes arithmetic product (in the sense of operator*) of its arguments.
 *
 * @param h the very first operand, distinct from the rest of the pack mostly to infer return type.
 * @param ts... al the rest operands
 * @return h * ts...
 */
template <typename H, typename... Ts>
constexpr auto Product(H &&h, Ts &&... ts) noexcept(
    noexcept(Accumulate(std::multiplies<void>{}, std::forward<H>(h), std::forward<Ts>(ts)...)))
{
  return Accumulate(std::multiplies<void>{}, std::forward<H>(h), std::forward<Ts>(ts)...);
}

/**
 * IsAnyOf tells if one of the rest arguments is equal (in the sense of operator==) to the head
 * argument. There's its namesake analog in namespace type_util.
 *
 * @param value the expression to be matched against all the patterns
 * @param ts... pattern pack
 * @return true if there's p among patterns such that value == p
 */
template <class T>
constexpr bool IsAnyOf(T &&) noexcept
{
  return false;
}

template <class T, class Last>
constexpr bool IsAnyOf(T &&value, Last &&pattern) noexcept(noexcept(std::forward<T>(value) ==
                                                                    std::forward<Last>(pattern)))
{
  return std::forward<T>(value) == std::forward<Last>(pattern);
}

template <class T, class H, class... Ts>
constexpr bool IsAnyOf(T &&value, H &&pattern,
                       Ts &&... patterns) noexcept(noexcept(no_op(value == pattern,
                                                                  value == patterns...)));

template <class T, class H, class... Ts>
constexpr bool IsAnyOf(T &&value, H &&pattern,
                       Ts &&... patterns) noexcept(noexcept(no_op(value == pattern,
                                                                  value == patterns...)))
{
  return value == std::forward<H>(pattern) ||
         IsAnyOf(std::forward<T>(value), std::forward<Ts>(patterns)...);
}

// Lisp-style boolean operators
namespace detail_ {

template <class H, class... Ts>
using BoolResultT = std::decay_t<type_util::LastT<H, Ts...>>;

template <class... Ts>
static constexpr bool IsNothrowAndableV = false;

template <class H, class... Ts>
static constexpr bool IsNothrowAndableV<H, Ts...> =
    noexcept(bool(std::declval<H>())) &&
    IsNothrowAndableV<Ts...> &&std::is_nothrow_constructible<BoolResultT<H, Ts...>>::value;

template <class L>
static constexpr bool IsNothrowAndableV<L> = std::is_nothrow_move_constructible<L>::value;

template <class... Ts>
static constexpr bool IsNothrowOrableV = false;

template <class H, class... Ts>
static constexpr bool IsNothrowOrableV<H, Ts...> =
    noexcept(bool(std::declval<H>())) && IsNothrowOrableV<Ts...>;

template <class L>
static constexpr bool IsNothrowOrableV<L> = std::is_nothrow_move_constructible<L>::value;

}  // namespace detail_

/**
 * And returns lisp-style conjunction of its arguments.
 * All (but the last) arguments should be coercible to bool.
 * As a special trick, the last argument needs not to.
 *
 * param values... pack of values
 * @return the first value to prove false in boolean context, or the last of the arguments if none
 */
constexpr bool And() noexcept
{
  return true;
}

template <class Last>
constexpr auto And(Last &&value) noexcept(std::is_nothrow_move_constructible<Last>::value)
{
  return std::forward<Last>(value);
}

template <class H, class... Ts>
constexpr auto And(H &&value,
                   Ts &&... values) noexcept(noexcept(detail_::IsNothrowAndableV<H, Ts...>));

template <class H, class... Ts>
constexpr auto And(H &&value,
                   Ts &&... values) noexcept(noexcept(detail_::IsNothrowAndableV<H, Ts...>))
{
  using RetVal = detail_::BoolResultT<H, Ts...>;
  return value ? And(std::forward<Ts>(values)...) : RetVal{};
}

/**
 * Or returns lisp-style disjunction of its arguments.
 * All (but the last) arguments should be coercible to bool.
 * As a special trick, the last argument needs not to.
 *
 * param values... pack of values
 * @return the first value to prove true in boolean context, or the last of the arguments if none
 */
constexpr bool Or() noexcept
{
  return true;
}

template <class Last>
constexpr auto Or(Last &&value) noexcept(std::is_nothrow_move_constructible<Last>::value)
{
  return std::forward<Last>(value);
}

template <class H, class... Ts>
constexpr auto Or(H &&value,
                  Ts &&... values) noexcept(noexcept(detail_::IsNothrowOrableV<H, Ts...>));

template <class H, class... Ts>
constexpr auto Or(H &&value,
                  Ts &&... values) noexcept(noexcept(detail_::IsNothrowOrableV<H, Ts...>))
{
  return value ? std::forward<H>(value) : Or(std::forward<Ts>(values)...);
}

/**
 * Invoke functor for each value of the pack.
 *
 * @param f a callable (either templated or overloaded) that can be invoked on each value of the
 * pack
 * @param values... a pack of values to invoke the function on
 */
template <class F>
constexpr void ForEach(F &&) noexcept
{}

template <class F, class T>
constexpr void ForEach(F &&f, T &&value)
{
  std::forward<F>(f)(std::forward<T>(value));
}

template <class F, class T, class... Ts>
constexpr void ForEach(F &&f, T &&value, Ts &&... values);

template <class F, class T, class... Ts>
constexpr void ForEach(F &&f, T &&value, Ts &&... values)
{
  f(std::forward<T>(value));
  ForEach(std::forward<F>(f), std::forward<Ts>(values)...);
}

/**
 * Reset (value-initialize) a single argument to zero
 */
struct ZeroOne
{
  template <class T>
  constexpr void operator()(T &&t) noexcept(noexcept(std::forward<T>(t) = std::decay_t<T>{}))
  {
    std::forward<T>(t) = std::decay_t<T>{};
  }
};

/**
 * Reset (value-initialize) a pack of arguments to zero each
 */
template <class... Ts>
constexpr void ZeroAll(Ts &&... ts) noexcept(noexcept(ForEach(ZeroOne{}, std::forward<Ts>(ts)...)))
{
  ForEach(ZeroOne{}, std::forward<Ts>(ts)...);
}

/**
 * Clear (either invoke member clear() or Clear()) on a single argument.
 *
 * @param t an instance of class with either method Clear() or clear() defined (but not both)
 * @return whatever member t.[Cc]lear() returns
 */
struct ClearOne
{
  template <class T>
  constexpr decltype(std::declval<T>().clear()) operator()(T &&t) const
      noexcept(noexcept(std::forward<T>(t).clear()))
  {
    return std::forward<T>(t).clear();
  }

  template <class T>
  constexpr decltype(std::declval<T>().Clear()) operator()(T &&t) const
      noexcept(noexcept(std::forward<T>(t).Clear()))
  {
    return std::forward<T>(t).Clear();
  }
};

/**
 * Clear (either invoke member clear() or Clear()) on a single argument.
 *
 * @param ts... instances of classes, each on having either method Clear() or clear() defined (but
 * not both)
 */
template <class... Ts>
constexpr void ClearAll(Ts &&... ts) noexcept(noexcept(ForEach(ClearOne{},
                                                               std::forward<Ts>(ts)...)))
{
  ForEach(ClearOne{}, std::forward<Ts>(ts)...);
}

/**
 * Invoke a function on arguments.
 * Special overloads are defined for member functions.
 *
 * @param f either a free a function, or a member function of the first of the (non-empty) args pack
 * @param args... possibly empty pack of arguments perfectly forwarded to f
 * @return whatever f returns
 */
template <class F, class... Args>
constexpr decltype(auto) Invoke(F &&f, Args &&... args)
{
  return std::forward<F>(f)(std::forward<Args>(args)...);
}

template <class RV, class C, class... Args1, class... Args>
constexpr decltype(auto) Invoke(RV (std::decay_t<C>::*f)(Args1...), C &&c, Args &&... args)
{
  return (std::forward<C>(c).*f)(std::forward<Args>(args)...);
}

template <class RV, class C, class... Args1, class... Args>
constexpr decltype(auto) Invoke(RV (std::decay_t<C>::*f)(Args1...) const, C &&c, Args &&... args)
{
  return (std::forward<C>(c).*f)(std::forward<Args>(args)...);
}

template <class RV, class C, class... Args1, class... Args>
constexpr decltype(auto) Invoke(RV (std::decay_t<C>::*f)(Args1...) &, C &&c, Args &&... args)
{
  return (std::forward<C>(c).*f)(std::forward<Args>(args)...);
}

template <class RV, class C, class... Args1, class... Args>
constexpr decltype(auto) Invoke(RV (std::decay_t<C>::*f)(Args1...) const &, C &&c, Args &&... args)
{
  return (std::forward<C>(c).*f)(std::forward<Args>(args)...);
}

template <class RV, class C, class... Args1, class... Args>
constexpr decltype(auto) Invoke(RV (std::decay_t<C>::*f)(Args1...) &&, C &&c, Args &&... args)
{
  return (std::forward<C>(c).*f)(std::forward<Args>(args)...);
}

template <class RV, class C, class... Args1, class... Args>
constexpr decltype(auto) Invoke(RV (std::decay_t<C>::*f)(Args1...) const &&, C &&c, Args &&... args)
{
  return (std::forward<C>(c).*f)(std::forward<Args>(args)...);
}

}  // namespace value_util
}  // namespace fetch
