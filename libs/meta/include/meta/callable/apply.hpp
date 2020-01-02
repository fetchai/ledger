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

#include "internal/classify_callable.hpp"
#include "meta/callable/callable_traits.hpp"
#include "meta/callable/invoke.hpp"
#include "meta/tuple.hpp"

#include <cstddef>
#include <tuple>
#include <type_traits>
#include <utility>

namespace fetch {
namespace meta {

namespace internal {

template <class F, typename Tuple, std::size_t... I>
constexpr decltype(auto) Apply(F &&f, Tuple &&t, IndexSequence<I...> && /*unused*/)
{
  return Invoke(std::forward<F>(f), std::get<I>(std::forward<Tuple>(t))...);
}

template <class F, typename Context, typename Tuple, std::size_t... I>
constexpr decltype(auto) Apply(F &&f, Context &&ctx, Tuple &&t, IndexSequence<I...> && /*unused*/)
{
  return Invoke(std::forward<F>(f), std::forward<Context>(ctx),
                std::get<I>(std::forward<Tuple>(t))...);
}

}  // namespace internal

template <typename F, typename Tuple>
constexpr decltype(auto) Apply(F &&f, Tuple &&tuple)
{
  static_assert(!CallableTraits<F>::is_non_static_member_function(),
                "To call non-static member functions, use the overloads with Context argument");

  static_assert(IsStdTuple<Tuple>, "Pass function arguments to Apply as a std::tuple");

  return internal::Apply(std::forward<F>(f), std::forward<Tuple>(tuple),
                         IndexSequenceFromTuple<Tuple>());
}

template <typename F, typename Context, typename Tuple>
constexpr decltype(auto) Apply(F &&f, Context &&ctx, Tuple &&tuple)
{
  static_assert(CallableTraits<F>::is_non_static_member_function(),
                "This overload is intended for calling non-static member functions only");
  static_assert(IsStdTuple<Tuple>, "Pass function arguments to Apply as a std::tuple");

  return internal::Apply(std::forward<F>(f), std::forward<Context>(ctx), std::forward<Tuple>(tuple),
                         meta::IndexSequenceFromTuple<Tuple>());
}

}  // namespace meta
}  // namespace fetch
