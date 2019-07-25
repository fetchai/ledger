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
template <class F, typename RV>
constexpr auto Accumulate(F &&, RV &&rv)
{
  return std::forward<RV>(rv);
}

template <class F, typename A, typename B, typename... Seq>
constexpr auto Accumulate(F &&f, A &&a, B &&b, Seq &&... seq);

// The recursion base: last step, only two values left.
template <class F, typename A, typename B>
constexpr auto Accumulate(F &&f, A &&a, B &&b)
{
  return std::forward<F>(f)(std::forward<A>(a), std::forward<B>(b));
}

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

}  // namespace value_util
}  // namespace fetch
