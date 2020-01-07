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

#include <tuple>
#include <type_traits>
#include <utility>

namespace fetch {
namespace meta {

namespace internal {

template <typename... Args>
struct MemberFunctionTraits;
// used for non-static member functions
template <typename R, typename C, typename... Args>
struct MemberFunctionTraits<R (C::*)(Args...) const>
{
  using OwningType    = C;
  using ReturnType    = R;
  using ArgsTupleType = std::tuple<Args...>;
};
template <typename R, typename C, typename... Args>
struct MemberFunctionTraits<R (C::*)(Args...)>
{
  using OwningType    = C;
  using ReturnType    = R;
  using ArgsTupleType = std::tuple<Args...>;
};
// used for functors
template <typename R, typename C, typename... Args>
struct MemberFunctionTraits<void, R (C::*)(Args...) const>
{
  using ReturnType    = R;
  using ArgsTupleType = std::tuple<Args...>;
};
template <typename R, typename C, typename... Args>
struct MemberFunctionTraits<void, R (C::*)(Args...)>
{
  using ReturnType    = R;
  using ArgsTupleType = std::tuple<Args...>;
};

template <typename F>
struct FreeOrStaticMemberFunctionTraits;
template <typename ReturnType_, typename... Args>
struct FreeOrStaticMemberFunctionTraits<ReturnType_ (*)(Args...)>
{
  using ReturnType    = ReturnType_;
  using ArgsTupleType = std::tuple<Args...>;
};

template <typename F, typename Tag>
struct CallableTraitsImpl;
template <typename F>
struct CallableTraitsImpl<F, free_or_static_member_fn_tag> : FreeOrStaticMemberFunctionTraits<F>
{
};
template <typename F>
struct CallableTraitsImpl<F, member_fn_tag> : MemberFunctionTraits<F>
{
};
template <typename F>
struct CallableTraitsImpl<F, const_member_fn_tag> : MemberFunctionTraits<F>
{
};
template <typename F>
struct CallableTraitsImpl<F, functor_tag>
  : MemberFunctionTraits<void, decltype(&std::remove_reference_t<F>::operator())>
{
};

template <typename F>
using CallableTraitsBase = CallableTraitsImpl<std::decay_t<F>, ClassifyCallable<F>>;

}  // namespace internal

/**
 * Accepts any callable type and provides the following public member types:
 *
 *   OwningType
 *     type to which callable belongs if callable is a non-static member
 *     function; undefined otherwise. Note in particular that static member
 *     functions have undefined OwningType.
 *   ReturnType
 *     type returned by callable. May be void
 *   ArgsTupleType
 *     std::tuple<Args...>, where Args are the argument types accepted
 *     by the callable; std::tuple<> for nullary callables
 *
 *   Free functions, static member functions, non-static member
 *   functions, lambdas and functors with one call operator are directly
 *   supported. Functors which overload the call operator must be used by
 *   passing in the pointer to the overload of interest, in effect treating
 *   it as a non-static member function:
 *
 *     struct SimpleFunctor
 *     {
 *       void operator()() {}
 *     };
 *     // this works
 *     using SimpleFunctorTraits1 = CallableTraits<SimpleFunctor>;
 *     using SimpleFunctorTraits2 = CallableTraits<&SimpleFunctor::operator()>;
 *
 *     struct OverloadedFunctor
 *     {
 *       void operator()() {}
 *       void operator()(int) const {}
 *     };
 *     // Error: ambiguous references to overloaded function
 *     //   OverloadedFunctor functor;
 *     //   using OverloadedFunctorTraits1 = CallableTraits<functor>;
 *     //   using OverloadedFunctorTraits2 =
 *     //     CallableTraits<&OverloadedFunctor::operator()>;
 *
 *     // this works
 *     using desired_overload = void (OverloadedFunctor::*)(int) const;
 *     using OverloadedFunctorTraits = CallableTraits<desired_overload>
 *
 * @tparam F callable type
 */
template <typename F>
struct CallableTraits : internal::CallableTraitsBase<F>
{
  /**
   * @return the number of arguments expected by F
   */
  constexpr static std::size_t arg_count()
  {
    return std::tuple_size<typename internal::CallableTraitsBase<F>::ArgsTupleType>::value;
  }

  /**
   * @return true if F return type is void; false otherwise
   */
  constexpr static bool is_void()
  {
    return std::is_void<typename internal::CallableTraitsBase<F>::ReturnType>::value;
  }

  /**
   * @return true if F is a member function; false otherwise
   */
  constexpr static bool is_non_static_member_function()
  {
    using tag = internal::ClassifyCallable<F>;

    return std::is_same<tag, internal::member_fn_tag>::value ||
           std::is_same<tag, internal::const_member_fn_tag>::value;
  }
};

}  // namespace meta
}  // namespace fetch
