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

/**
 * Accumulate(f, a0, a1, a2, ..., an) returns f(f(...(f(a0, a1), a2), ...), an).
 * It is a left-fold, similar to std::accumulate, but operating on argument packs rather than
 * ranges.
 */

// The zero case: the pack is empty past a0.
template <typename F, typename RV>
constexpr auto Accumulate(F && /*never_called*/, RV &&rv)
{
  return std::forward<RV>(rv);
}

// The recursion base: last step, only two values left.
template <typename F, typename A, typename B>
constexpr auto Accumulate(F &&f, A &&a, B &&b)
{
  return std::forward<F>(f)(std::forward<A>(a), std::forward<B>(b));
}

// The generic case.
template <typename F, typename A, typename B, typename... Seq>
constexpr auto Accumulate(F &&f, A &&a, B &&b, Seq &&... seq);

template <typename F, typename A, typename B, typename... Seq>
constexpr auto Accumulate(F &&f, A &&a, B &&b, Seq &&... seq)
{
  return Accumulate(std::forward<F>(f), f(std::forward<A>(a), std::forward<B>(b)),
                    std::forward<Seq>(seq)...);
}

template <class F>
constexpr void ForEach(F && /*f*/) noexcept
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
  f(std::forward<T>(t));
  ForEach(std::forward<F>(f), std::forward<Ts>(ts)...);
}

struct ZeroOne
{
  template <class T>
  constexpr void operator()(T &&t) const
  {
    std::forward<T>(t) = std::decay_t<T>{};
  }
};

template <class... Ts>
constexpr void ZeroAll(Ts &&... ts)
{
  ForEach(ZeroOne{}, std::forward<Ts>(ts)...);
}

namespace detail_ {

struct Yes
{
};
struct No
{
};

template <class T>
class NonClearable
{
  template <class U>
  static constexpr auto Can(U &&u) -> decltype(u.clear(), Yes{});
  template <class U>
  static constexpr auto Can(U &&u) -> decltype(u.Clear(), Yes{});

  static constexpr No Can(...);

public:
  enum : bool
  {
    value = std::is_same<decltype(Can(std::declval<T>())), No>::value
  };
};

template <class T>
static constexpr auto NonClearableV = NonClearable<std::decay_t<T>>::value;

template <class T>
class NonResettable
{
  template <class U>
  static constexpr auto Can(U &&u) -> decltype(u.clear(), Yes{});
  template <class U>
  static constexpr auto Can(U &&u) -> decltype(u.Resett(), Yes{});

  static constexpr No Can(...);

public:
  enum : bool
  {
    value = std::is_same<decltype(Can(std::declval<T>())), No>::value
  };
};

template <class T>
static constexpr auto NonResettableV = NonResettable<std::decay_t<T>>::value;

}  // namespace detail_

struct ClearOne
{
  template <class T>
  constexpr decltype(std::declval<T>().clear()) operator()(T &&t) const
  {
    return std::forward<T>(t).clear();
  }

  template <class T>
  constexpr decltype(std::declval<T>().Clear()) operator()(T &&t) const
  {
    return std::forward<T>(t).Clear();
  }

  template <class T>
  constexpr std::enable_if_t<detail_::NonClearableV<T>> operator()(T &&t) const
  {
    ZeroOne{}(std::forward<T>(t));
  }
};

template <class... Ts>
constexpr void ClearAll(Ts &&... ts)
{
  ForEach(ClearOne{}, std::forward<Ts>(ts)...);
}

struct ResetOne
{
  template <class T>
  constexpr decltype(std::declval<T>().clear()) operator()(T &&t) const
  {
    return std::forward<T>(t).clear();
  }

  template <class T>
  constexpr decltype(std::declval<T>().Reset()) operator()(T &&t) const
  {
    return std::forward<T>(t).Reset();
  }

  template <class T>
  constexpr std::enable_if_t<detail_::NonResettableV<T>> operator()(T &&t) const
  {
    ZeroOne{}(std::forward<T>(t));
  }
};

template <class... Ts>
constexpr void ResetAll(Ts &&... ts)
{
  ForEach(ResetOne{}, std::forward<Ts>(ts)...);
}

}  // namespace value_util
}  // namespace fetch
