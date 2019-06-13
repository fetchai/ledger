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

#include "meta/type_util.hpp"

#include <functional>
#include <type_traits>
#include <utility>

namespace fetch {
namespace value_util {

namespace detail_ {
template <class F, typename...>
struct IsNothrowAccumulatable;
template <class F, typename... Seq>
static constexpr auto IsNothrowAccumulatableV = IsNothrowAccumulatable<F, Seq...>::value;

template <class F, class A, class B, class... Tail>
struct IsNothrowAccumulatable<F, A, B, Tail...>
{
  enum : bool
  {
    value = type_util::IsNothrowInvocableV<std::add_lvalue_reference_t<F>, A, B> &&
            IsNothrowAccumulatableV<F, type_util::InvokeResultT<F, A, B>, Tail...>
  };
};

template <class F, class A, class B>
struct IsNothrowAccumulatable<F, A, B> : type_util::IsNothrowInvocable<F, A, B>
{
};

template <class F, class A>
struct IsNothrowAccumulatable<F, A> : std::is_nothrow_move_constructible<A>
{
};
}  // namespace detail_

/**
 * Accumulate(f, a0, a1, a2, ..., an) returns f(f(...(f(a0, a1), a2), ...), an).
 * It is a left-fold, similar to std::accumulate, but operating on argument packs rather than
 * ranges.
 */

// The zero case: the pack is empty past a0.
template <class F, typename RV>
constexpr auto Accumulate(F &&, RV &&rv) noexcept(detail_::IsNothrowAccumulatableV<F, RV>)
{
  return rv;
}

template <class F, typename A, typename B, typename... Seq>
constexpr auto Accumulate(F &&f, A &&a, B &&b, Seq &&... seq) noexcept(
    detail_::IsNothrowAccumulatableV<F, A, B, Seq...>);

// The recursion base: last step, only two values left.
template <class F, typename A, typename B>
constexpr auto Accumulate(F &&f, A &&a, B &&b) noexcept(detail_::IsNothrowAccumulatableV<F, A, B>)
{
  return std::forward<F>(f)(std::forward<A>(a), std::forward<B>(b));
}

// The generic case.
template <class F, typename A, typename B, typename... Seq>
constexpr auto Accumulate(F &&f, A &&a, B &&b,
                          Seq &&... seq) noexcept(detail_::IsNothrowAccumulatableV<F, A, B, Seq...>)
{
  return Accumulate(std::forward<F>(f), f(std::forward<A>(a), std::forward<B>(b)),
                    std::forward<Seq>(seq)...);
}

// Simple uses of Accumulate().
// Since Accumulate() itself operates on packs rather than ranges,
// there's no special overload to calculate simple sum, like in STL.
template <typename H, typename... Ts>
constexpr auto Sum(H &&h, Ts &&... ts) noexcept(
    detail_::IsNothrowAccumulatableV<std::plus<void>, H, Ts...>)
{
  return Accumulate(std::plus<void>{}, std::forward<H>(h), std::forward<Ts>(ts)...);
}

template <typename H, typename... Ts>
constexpr auto Product(H &&h, Ts &&... ts) noexcept(
    detail_::IsNothrowAccumulatableV<std::multiplies<void>, H, Ts...>)
{
  return Accumulate(std::multiplies<void>{}, std::forward<H>(h), std::forward<Ts>(ts)...);
}

template <class LHS, class RHS>
struct IsNothrowComparable
{
  enum : bool
  {
    value = noexcept(std::declval<LHS>() == std::declval<RHS>())
  };
};

template <class LHS, class RHS>
static constexpr bool IsNothrowComparableV = IsNothrowComparable<LHS, RHS>::value;

template <class T>
inline constexpr bool IsAnyOf(T &&) noexcept
{
  return false;
}

template <class T, class Last>
inline constexpr bool IsAnyOf(T &&value, Last &&last) noexcept(IsNothrowComparableV<T, Last>)
{
  return std::forward<T>(value) == std::forward<Last>(last);
}

template <class T, class H, class... Ts>
inline constexpr bool IsAnyOf(T &&value, H &&h, Ts &&... ts) noexcept(
    type_util::AllV<type_util::Bind<IsNothrowComparable, T>::template type, H, Ts...>);

template <class T, class H, class... Ts>
inline constexpr bool IsAnyOf(T &&value, H &&h, Ts &&... ts) noexcept(
    type_util::AllV<type_util::Bind<IsNothrowComparable, T>::template type, H, Ts...>)
{
  return value == std::forward<H>(h) || IsAnyOf(std::forward<T>(value), std::forward<Ts>(ts)...);
}

constexpr bool And() noexcept
{
  return true;
}

template <class Last>
inline constexpr std::decay_t<Last> And(Last &&last) noexcept(
    std::is_nothrow_move_constructible<Last>::value)
{
  return std::forward<Last>(last);
}

template <class H, class... Ts>
inline constexpr auto And(H &&h, Ts &&... ts) noexcept(
    noexcept(bool(std::declval<std::add_lvalue_reference_t<H>>())) &&
    type_util::AllV<type_util::Bind<std::is_nothrow_constructible, bool>::template type, Ts...> &&
    std::is_nothrow_constructible<std::decay_t<type_util::LastT<H, Ts...>>>::value)
{
  using RetVal = std::decay_t<type_util::LastT<H, Ts...>>;
  return h ? And(std::forward<Ts>(ts)...) : RetVal{};
}

constexpr bool Or() noexcept
{
  return false;
}

template <class Last>
inline constexpr auto Or(Last &&last) noexcept(
    std::is_nothrow_constructible<typename std::decay<Last>::type, Last>::value)
{
  return std::forward<Last>(last);
}

template <class H, class... Ts>
inline constexpr auto Or(H &&h, Ts &&... ts) noexcept(
    noexcept(bool(std::declval < std::add_lvalue_reference<H>::type)) &&
    type_util::AllV<type_util::Bind<std::is_nothrow_constructible, bool>::template type, Ts...> &&
    std::is_nothrow_constructible<std::decay_t<type_util::LastT<H, Ts...>>>::value)
{
  using RetVal = std::decay_t<type_util::LastT<H, Ts...>>;
  return h ? RetVal(std::forward<H>(h)) : Or(std::forward<Ts>(ts)...);
}

template <class F>
inline constexpr void ForEach(F &&) noexcept
{}

template <class F, class T>
inline constexpr void ForEach(F &&f, T &&t) noexcept(type_util::IsNothrowInvocableV<F, T>)
{
  std::forward<F>(f)(std::forward<T>(t));
}

template <class F, class T, class... Ts>
inline constexpr void ForEach(F &&f, T &&t, Ts &&... ts) noexcept(
    type_util::AllV<type_util::Bind<type_util::IsNothrowInvocable, F>::template type, T, Ts...>);

template <class F, class T, class... Ts>
inline constexpr void ForEach(F &&f, T &&t, Ts &&... ts) noexcept(
    type_util::AllV<type_util::Bind<type_util::IsNothrowInvocable, F>::template type, T, Ts...>)
{
  f(std::forward<T>(t)), ForEach(std::forward<F>(f), std::forward<Ts>(ts)...);
}

struct ResetOne
{
  template <class T>
  inline constexpr void operator()(T &&t) noexcept(
      std::is_nothrow_constructible<std::decay_t<T>>::value
          &&std::is_nothrow_move_assignable<std::decay_t<T>>::value)
  {
    t = std::decay_t<T>{};
  }
};

template <class... Ts>
inline constexpr void ResetAll(Ts &&... ts) noexcept(noexcept(ForEach(ResetOne{},
                                                                      std::forward<Ts>(ts)...)))
{
  ForEach(ResetOne{}, std::forward<Ts>(ts)...);
}

struct ClearOne
{
  template <class T>
  inline constexpr void operator()(T &&t) noexcept(noexcept(std::forward<T>(t).clear()))
  {
    std::forward<T>(t).clear();
  }
};

template <class... Ts>
inline constexpr void ClearAll(Ts &&... ts) noexcept(noexcept(ForEach(ClearOne{},
                                                                      std::forward<Ts>(ts)...)))
{
  ForEach(ClearOne{}, std::forward<Ts>(ts)...);
}

template <class... Ts>
constexpr void no_op(Ts &&... /*ts*/) noexcept
{}

}  // namespace value_util
}  // namespace fetch
