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

namespace internal {

template <typename F, typename...>
struct IsNothrowAccumulatable;
template <typename F, typename... Seq>
static constexpr auto IsNothrowAccumulatableV = IsNothrowAccumulatable<F, Seq...>::value;

template <typename F, typename A, typename B, typename... Tail>
struct IsNothrowAccumulatable<F, A, B, Tail...>
{
  enum : bool
  {
    value = type_util::IsNothrowInvocableV<std::add_lvalue_reference_t<F>, A, B> &&
            IsNothrowAccumulatableV<F, type_util::InvokeResultT<F, A, B>, Tail...>
  };
};

template <typename F, typename A, typename B>
struct IsNothrowAccumulatable<F, A, B> : type_util::IsNothrowInvocable<F, A, B>
{
};

template <typename F, typename A>
struct IsNothrowAccumulatable<F, A> : std::is_nothrow_move_constructible<A>
{
};

}  // namespace internal

/**
 * Accumulate(f, a0, a1, a2, ..., an) returns f(f(...(f(a0, a1), a2), ...), an).
 * It is a left-fold, similar to std::accumulate, but operating on argument packs rather than
 * ranges.
 */

// The zero case: the pack is empty past a0.
template <typename F, typename RV>
constexpr auto Accumulate(F && /*unused*/,
                          RV &&rv) noexcept(internal::IsNothrowAccumulatableV<F, RV>)
{
  return rv;
}

template <typename F, typename A, typename B, typename... Seq>
constexpr auto Accumulate(F &&f, A &&a, B &&b, Seq &&... seq) noexcept(
    internal::IsNothrowAccumulatableV<F, A, B, Seq...>);

// The recursion base: last step, only two values left.
template <typename F, typename A, typename B>
constexpr auto Accumulate(F &&f, A &&a, B &&b) noexcept(internal::IsNothrowAccumulatableV<F, A, B>)
{
  return std::forward<F>(f)(std::forward<A>(a), std::forward<B>(b));
}

// The generic case.
template <typename F, typename A, typename B, typename... Seq>
constexpr auto Accumulate(F &&f, A &&a, B &&b, Seq &&... seq) noexcept(
    internal::IsNothrowAccumulatableV<F, A, B, Seq...>)
{
  return Accumulate(std::forward<F>(f), f(std::forward<A>(a), std::forward<B>(b)),
                    std::forward<Seq>(seq)...);
}

}  // namespace value_util
}  // namespace fetch
