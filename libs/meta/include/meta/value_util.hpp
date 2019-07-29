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

template <class F, typename A, typename B, typename... Seq>
constexpr auto Accumulate(F &&f, A &&a, B &&b, Seq &&... seq);

// The generic case.
template <class F, typename A, typename B, typename... Seq>
constexpr auto Accumulate(F &&f, A &&a, B &&b, Seq &&... seq)
{
  return Accumulate(std::forward<F>(f), f(std::forward<A>(a), std::forward<B>(b)),
                    std::forward<Seq>(seq)...);
}

// Simple uses of Accumulate().
// Since Accumulate() itself operates on packs rather than ranges,
// there's no special overload to calculate simple sum, like in STL.
template <typename H, typename... Ts>
constexpr auto Sum(H &&h, Ts &&... ts)
{
  return Accumulate(std::plus<void>{}, std::forward<H>(h), std::forward<Ts>(ts)...);
}

template <typename H, typename... Ts>
constexpr auto Product(H &&h, Ts &&... ts)
{
  return Accumulate(std::multiplies<void>{}, std::forward<H>(h), std::forward<Ts>(ts)...);
}

template <class T>
constexpr bool IsAnyOf(T &&) noexcept
{
  return false;
}

template <class T, class Last>
constexpr bool IsAnyOf(T &&value, Last &&last) noexcept(noexcept(std::forward<T>(value) ==
                                                                 std::forward<Last>(last)))
{
  return std::forward<T>(value) == std::forward<Last>(last);
}

// TODO(bipll): here and below, noexcept specifications are not actually correct and need to be
// fixed
template <class T, class H, class... Ts>
constexpr bool IsAnyOf(T &&value, H &&h, Ts &&... ts);

template <class T, class H, class... Ts>
constexpr bool IsAnyOf(T &&value, H &&h, Ts &&... ts)
{
  return value == std::forward<H>(h) || IsAnyOf(std::forward<T>(value), std::forward<Ts>(ts)...);
}

constexpr bool And() noexcept
{
  return true;
}

template <class Last>
constexpr std::decay_t<Last> And(Last &&last) noexcept(
    std::is_nothrow_move_constructible<Last>::value)
{
  return std::forward<Last>(last);
}

template <class H, class... Ts>
constexpr auto And(H &&h, Ts &&... ts)
{
  using RetVal = std::decay_t<type_util::LastT<H, Ts...>>;
  return h ? And(std::forward<Ts>(ts)...) : RetVal{};
}

constexpr bool Or() noexcept
{
  return false;
}

template <class Last>
constexpr auto Or(Last &&last) noexcept(
    std::is_nothrow_constructible<typename std::decay<Last>::type, Last>::value)
{
  return std::forward<Last>(last);
}

template <class H, class... Ts>
constexpr auto Or(H &&h, Ts &&... ts)
{
  using RetVal = std::decay_t<type_util::LastT<H, Ts...>>;
  return h ? RetVal(std::forward<H>(h)) : Or(std::forward<Ts>(ts)...);
}

template <class F>
constexpr void ForEach(F &&) noexcept
{}

template <class F, class T>
constexpr void ForEach(F &&f, T &&t)
{
  std::forward<F>(f)(std::forward<T>(t));
}

template <class F, class T, class... Ts>
constexpr void ForEach(F &&f, T &&t, Ts &&... ts);

template <class F, class T, class... Ts>
constexpr void ForEach(F &&f, T &&t, Ts &&... ts)
{
  f(std::forward<T>(t)), ForEach(std::forward<F>(f), std::forward<Ts>(ts)...);
}

struct ZeroOne
{
  template <class T>
  constexpr void operator()(T &&t) noexcept(noexcept(std::forward<T>(t) = std::decay_t<T>{}))
  {
    std::forward<T>(t) = std::decay_t<T>{};
  }
};

template <class... Ts>
constexpr void ZeroAll(Ts &&... ts) noexcept(noexcept(ForEach(ZeroOne{}, std::forward<Ts>(ts)...)))
{
  ForEach(ZeroOne{}, std::forward<Ts>(ts)...);
}

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

template <class... Ts>
constexpr void ClearAll(Ts &&... ts) noexcept(noexcept(ForEach(ClearOne{},
                                                               std::forward<Ts>(ts)...)))
{
  ForEach(ClearOne{}, std::forward<Ts>(ts)...);
}

template <class... Ts>
constexpr void no_op(Ts &&... /*ts*/) noexcept
{}

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

template <class... StoredTypes>
class KeptValues
{
  using StorageType = std::tuple<StoredTypes...>;
  using RecoverType = std::tuple<StoredTypes &...>;

  StorageType storage_;
  RecoverType recover_;

public:
  constexpr KeptValues(StoredTypes &... values) noexcept(
      std::is_nothrow_constructible<StorageType, StoredTypes &...>::value
          &&std::is_nothrow_constructible<RecoverType, StoredTypes &...>::value)
    : storage_{values...}
    , recover_{values...}
  {}

  ~KeptValues()
  {
    recover_ = std::move(storage_);
  }
};
template <class... StoredTypes>
inline constexpr KeptValues<StoredTypes...> KeepValues(StoredTypes &... values) noexcept(
    std::is_nothrow_constructible<KeptValues<StoredTypes...>, StoredTypes &...>::value)
{
  return KeptValues<StoredTypes...>(values...);
}

}  // namespace value_util
}  // namespace fetch
