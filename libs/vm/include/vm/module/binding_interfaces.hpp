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

#include "meta/callable/callable_traits.hpp"
#include "meta/tuple.hpp"
#include "vm/estimate_charge.hpp"
#include "vm/module/base.hpp"
#include "vm/module/constructor_invoke.hpp"
#include "vm/module/free_function_invoke.hpp"
#include "vm/module/member_function_invoke.hpp"
#include "vm/module/prepare_invocation.hpp"
#include "vm/module/static_member_function_invoke.hpp"
#include "vm/object.hpp"
#include "vm/vm.hpp"

#include <type_traits>
#include <utility>

namespace fetch {
namespace vm {

namespace internal {

template<typename T, typename Tuple>
struct PrependedTupleImpl;

template<typename T, typename... Ts>
struct PrependedTupleImpl<T, std::tuple<Ts...>>{
using type = std::tuple<T, Ts...>;
};

template <typename T, typename Tuple>
using PrependedTuple = typename PrependedTupleImpl<T, Tuple>::type;

template <template <int, typename, typename, typename...> class Invoker, typename EtchArgsTuple>
struct EtchMemberFunctionInvocation
{
  static_assert(meta::IsStdTuple<EtchArgsTuple>, "Pass binding argument types as a std::tuple");

  template <typename Estimator, typename Callable, typename... ExtraArgs>
  static void InvokeHandler(VM *vm, Estimator &&estimator, Callable &&callable,
                            ExtraArgs const &... extra_args)
  {
  using OwningType = typename meta::CallableTraits<Callable>::OwningType;

  constexpr static int num_parameters = int(std::tuple_size<EtchArgsTuple>::value);
  constexpr static int sp_offset =
      meta::CallableTraits<Callable>::is_void() ? num_parameters : (num_parameters - 1);

  constexpr static int offset =
      meta::CallableTraits<Callable>::is_void() ? sp_offset -1: sp_offset;

    using Config = PrepareInvocation<Invoker, Callable, EtchArgsTuple, std::tuple<ExtraArgs...>>;

    // get the Etch argument values from the stack
    auto etch_args_tuple = Config::GetEtchArguments(vm);

    Variant &       v      = vm->stack_[vm->sp_ - offset];
    Ptr<OwningType> context = v.object;

    using EstimatorArgsTuple = PrependedTuple<Ptr<OwningType>, EtchArgsTuple>;
    
    auto estimator_args_tuple =
          std::tuple_cat(std::make_tuple(context), std::move(etch_args_tuple));
    
    using EstimatorType = meta::UnpackTuple<EstimatorArgsTuple, ChargeEstimator>;
    if (EstimateCharge(vm, EstimatorType{std::forward<Estimator>(estimator)}, estimator_args_tuple))
    {
      // prepend extra non-etch arguments (e.g. VM*, TypeId)
      auto all_args_tuple =
          std::tuple_cat(std::make_tuple(extra_args...), std::move(etch_args_tuple));

      // Invoke C++ handler
      using ConfiguredInvoker = typename Config::ConfiguredInvoker;
      ConfiguredInvoker::Invoke(vm, std::move(all_args_tuple), std::forward<Callable>(callable));
    }
  }
};

template <template <int, typename, typename, typename...> class Invoker, typename EtchArgsTuple>
struct EtchInvocation
{
  static_assert(meta::IsStdTuple<EtchArgsTuple>, "Pass binding argument types as a std::tuple");

  template <typename Estimator, typename Callable, typename... ExtraArgs>
  static void InvokeHandler(VM *vm, Estimator &&estimator, Callable &&callable,
                            ExtraArgs const &... extra_args)
  {
    using Config = PrepareInvocation<Invoker, Callable, EtchArgsTuple, std::tuple<ExtraArgs...>>;

    // get the Etch argument values from the stack
    auto etch_args_tuple = Config::GetEtchArguments(vm);

    using EstimatorType = meta::UnpackTuple<EtchArgsTuple, ChargeEstimator>;
    if (EstimateCharge(vm, EstimatorType{std::forward<Estimator>(estimator)}, etch_args_tuple))
    {
      // prepend extra non-etch arguments (e.g. VM*, TypeId)
      auto all_args_tuple =
          std::tuple_cat(std::make_tuple(extra_args...), std::move(etch_args_tuple));

      // Invoke C++ handler
      using ConfiguredInvoker = typename Config::ConfiguredInvoker;
      ConfiguredInvoker::Invoke(vm, std::move(all_args_tuple), std::forward<Callable>(callable));
    }
  }
};

}  // namespace internal

template <typename EtchArgsTuple>
using Constructor = internal::EtchInvocation<VmConstructorInvoker, EtchArgsTuple>;
template <typename EtchArgsTuple>
using FreeFunction = internal::EtchInvocation<VmFreeFunctionInvoker, EtchArgsTuple>;
template <typename EtchArgsTuple>
using NewMemberFunction = internal::EtchMemberFunctionInvocation<VmMemberFunctionInvoker, EtchArgsTuple>;
template <typename EtchArgsTuple>
using MemberFunction = internal::EtchInvocation<VmMemberFunctionInvoker, EtchArgsTuple>;
template <typename EtchArgsTuple>
using StaticMemberFunction = internal::EtchInvocation<VmStaticMemberFunctionInvoker, EtchArgsTuple>;

}  // namespace vm
}  // namespace fetch
