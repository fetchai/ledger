#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018-2020 Fetch.AI Limited
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
constexpr auto Accumulate(F && /*unused*/, RV &&rv)
{
  return rv;
}

template <typename F, typename A, typename B, typename... Seq>
constexpr auto Accumulate(F &&f, A &&a, B &&b, Seq &&... seq);

// The recursion base: last step, only two values left.
template <typename F, typename A, typename B>
constexpr auto Accumulate(F &&f, A &&a, B &&b)
{
  return std::forward<F>(f)(std::forward<A>(a), std::forward<B>(b));
}

// The generic case.
template <typename F, typename A, typename B, typename... Seq>
constexpr auto Accumulate(F &&f, A &&a, B &&b, Seq &&... seq)
{
  return Accumulate(std::forward<F>(f), f(std::forward<A>(a), std::forward<B>(b)),
                    std::forward<Seq>(seq)...);
}

template <class... Ts>
constexpr auto Sum(Ts &&... ts)
{
  return Accumulate(std::plus<void>{}, std::forward<Ts>(ts)...);
}

template <class T>
constexpr std::add_const_t<T> &AsConst(T const &t)
{
  return t;
}

template <class Constant = void, class...>
struct NoOp
{
  template <class... Ts>
  constexpr auto operator()(Ts &&... /*unused*/) const noexcept
  {
    return Constant::value;
  }
};

template <class... Rest>
struct NoOp<void, Rest...>
{
  template <class... Ts>
  constexpr void operator()(Ts &&... /*unused*/) const noexcept
  {}
};

template <class F>
constexpr void ForEach(F &&f) noexcept
{
  return NoOp<>{}(std::forward<F>(f));
}

template <class F, class T1>
constexpr auto ForEach(F &&f, T1 &&t1)
{
  return std::forward<F>(f)(std::forward<T1>(t1));
}

template <class F, class T1, class T2>
constexpr auto ForEach(F &&f, T1 &&t1, T2 &&t2)
{
  f(std::forward<T1>(t1));
  return std::forward<F>(f)(std::forward<T2>(t2));
}

template <class F, class T1, class T2, class T3>
constexpr auto ForEach(F &&f, T1 &&t1, T2 &&t2, T3 &&t3)
{
  f(std::forward<T1>(t1));
  f(std::forward<T2>(t2));
  return std::forward<F>(f)(std::forward<T3>(t3));
}

template <class F, class T1, class T2, class T3, class T4>
constexpr auto ForEach(F &&f, T1 &&t1, T2 &&t2, T3 &&t3, T4 &&t4)
{
  f(std::forward<T1>(t1));
  f(std::forward<T2>(t2));
  f(std::forward<T3>(t3));
  return std::forward<F>(f)(std::forward<T4>(t4));
}

template <class F, class T1, class T2, class T3, class T4, class... Ts>
constexpr auto ForEach(F &&f, T1 &&t1, T2 &&t2, T3 &&t3, T4 &&t4, Ts &&... ts);

template <class F, class T1, class T2, class T3, class T4, class... Ts>
constexpr auto ForEach(F &&f, T1 &&t1, T2 &&t2, T3 &&t3, T4 &&t4, Ts &&... ts)
{
  f(std::forward<T1>(t1));
  f(std::forward<T2>(t2));
  f(std::forward<T3>(t3));
  f(std::forward<T4>(t4));
  ForEach(std::forward<F>(f), std::forward<Ts>(ts)...);
}

}  // namespace value_util
}  // namespace fetch
