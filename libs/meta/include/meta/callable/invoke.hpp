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

#include <type_traits>
#include <utility>

namespace fetch {
namespace meta {

namespace internal {

template <typename>
struct CallableCategory;
template <>
struct CallableCategory<free_or_static_member_fn_tag>
{
  template <typename F, typename... Args>
  static constexpr decltype(auto) Invoke(F &&f, Args &&... args)
  {
    return std::forward<F>(f)(std::forward<Args>(args)...);
  }
};
template <>
struct CallableCategory<functor_tag>
{
  template <typename F, typename... Args>
  static constexpr decltype(auto) Invoke(F &&f, Args &&... args)
  {
    return std::forward<F>(f)(std::forward<Args>(args)...);
  }
};

// Context passed as pointer
template <typename F, typename Context, typename... Args>
constexpr decltype(auto) InvokeNonStaticMemberFn_(F &&f, Context *ctx, Args &&... args)
{
  using OwningType = typename CallableTraits<F>::OwningType;
  static_assert(std::is_base_of<OwningType, std::decay_t<Context>>::value,
                "Called member function and context do not match");

  return (ctx->*f)(std::forward<Args>(args)...);
}

// Context passed as reference
template <typename F, typename Context, typename... Args>
constexpr decltype(auto) InvokeNonStaticMemberFn_(F &&f, Context &&ctx, Args &&... args)
{
  using OwningType = typename CallableTraits<F>::OwningType;
  static_assert(std::is_base_of<OwningType, std::remove_reference_t<Context>>::value,
                "Called member function and context do not match");

  return (std::forward<Context>(ctx).*f)(std::forward<Args>(args)...);
}
template <>
struct CallableCategory<const_member_fn_tag>
{
  template <typename F, typename Context, typename... Args>
  static constexpr decltype(auto) Invoke(F &&f, Context &&ctx, Args &&... args)
  {
    return InvokeNonStaticMemberFn_(std::forward<F>(f), std::forward<Context>(ctx),
                                    std::forward<Args>(args)...);
  }
};
template <>
struct CallableCategory<member_fn_tag>
{
  template <typename F, typename Context, typename... Args>
  static constexpr decltype(auto) Invoke(F &&f, Context &&ctx, Args &&... args)
  {
    return InvokeNonStaticMemberFn_(std::forward<F>(f), std::forward<Context>(ctx),
                                    std::forward<Args>(args)...);
  }
};

}  // namespace internal

template <typename F, typename... Args>
constexpr decltype(auto) Invoke(F &&f, Args &&... args)
{
  using tag = internal::ClassifyCallable<F>;

  return internal::CallableCategory<tag>::Invoke(std::forward<F>(f), std::forward<Args>(args)...);
}

}  // namespace meta
}  // namespace fetch
