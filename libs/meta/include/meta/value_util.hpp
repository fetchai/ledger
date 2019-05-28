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

template <class F, typename RV>
constexpr auto Accumulate(F &&, RV &&rv) noexcept(detail_::IsNothrowAccumulatableV<F, RV>)
{
  return rv;
}

template <class F, typename A, typename B, typename... Seq>
constexpr auto Accumulate(F &&f, A &&a, B &&b, Seq &&... seq) noexcept(
    detail_::IsNothrowAccumulatableV<F, A, B, Seq...>);

template <class F, typename A, typename B>
constexpr auto Accumulate(F &&f, A &&a, B &&b) noexcept(detail_::IsNothrowAccumulatableV<F, A, B>);

template <class F, typename A, typename B, typename... Seq>
constexpr auto Accumulate(F &&f, A &&a, B &&b,
                          Seq &&... seq) noexcept(detail_::IsNothrowAccumulatableV<F, A, B, Seq...>)
{
  return Accumulate(std::forward<F>(f), f(std::forward<A>(a), std::forward<B>(b)),
                    std::forward<Seq>(seq)...);
}

template <class F, typename A, typename B>
constexpr auto Accumulate(F &&f, A &&a, B &&b) noexcept(detail_::IsNothrowAccumulatableV<F, A, B>)
{
  return std::forward<F>(f)(std::forward<A>(a), std::forward<B>(b));
}

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

}  // namespace value_util
}  // namespace fetch
