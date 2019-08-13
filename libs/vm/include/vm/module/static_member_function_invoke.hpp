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

#include "vm/estimate_charge.hpp"

namespace fetch {
namespace vm {

template <typename Estimator, typename ReturnType, typename StaticMemberFunction, typename... Ts>
struct StaticMemberFunctionInvokerHelper
{
  static void Invoke(VM *vm, int sp_offset, TypeId type_id, TypeId return_type_id,
                     Estimator const &e, StaticMemberFunction const &f, Ts const &... parameters)
  {
    if (EstimateCharge(vm, {e}, parameters...))
    {
      ReturnType result((*f)(vm, type_id, parameters...));
      StackSetter<ReturnType>::Set(vm, sp_offset, std::move(result), return_type_id);
      vm->sp_ -= sp_offset;
    }
  };
};

template <typename Estimator, typename StaticMemberFunction, typename... Ts>
struct StaticMemberFunctionInvokerHelper<Estimator, void, StaticMemberFunction, Ts...>
{
  static void Invoke(VM *vm, int sp_offset, TypeId type_id, TypeId /* return_type_id */,
                     Estimator const &e, StaticMemberFunction const &f, Ts const &... parameters)
  {
    if (EstimateCharge(vm, {e}, parameters...))
    {
      (*f)(vm, type_id, parameters...);
      vm->sp_ -= sp_offset;
    }
  };
};

template <typename Estimator, typename ReturnType, typename StaticMemberFunction, typename... Used>
struct StaticMemberFunctionInvoker
{
  template <int PARAMETER_OFFSET, typename... Ts>
  struct Invoker;
  template <int PARAMETER_OFFSET, typename T, typename... Ts>
  struct Invoker<PARAMETER_OFFSET, T, Ts...>
  {
    // Invoked on non-final parameter
    static void Invoke(VM *vm, int sp_offset, TypeId type_id, TypeId return_type_id,
                       Estimator const &e, StaticMemberFunction const &f, Used const &... used)
    {
      using P = std::decay_t<T>;
      P parameter(StackGetter<P>::Get(vm, PARAMETER_OFFSET));
      using InvokerType =
          typename StaticMemberFunctionInvoker<Estimator, ReturnType, StaticMemberFunction, Used...,
                                               T>::template Invoker<PARAMETER_OFFSET - 1, Ts...>;
      InvokerType::Invoke(vm, sp_offset, type_id, return_type_id, e, f, used..., parameter);
    }
  };
  template <int PARAMETER_OFFSET, typename T>
  struct Invoker<PARAMETER_OFFSET, T>
  {
    // Invoked on final parameter
    static void Invoke(VM *vm, int sp_offset, TypeId type_id, TypeId return_type_id,
                       Estimator const &e, StaticMemberFunction const &f, Used const &... used)
    {
      using P = std::decay_t<T>;
      P parameter(StackGetter<P>::Get(vm, PARAMETER_OFFSET));
      using InvokerType = StaticMemberFunctionInvokerHelper<Estimator, ReturnType,
                                                            StaticMemberFunction, Used..., T>;
      InvokerType::Invoke(vm, sp_offset, type_id, return_type_id, e, f, used..., parameter);
    }
  };
  template <int PARAMETER_OFFSET>
  struct Invoker<PARAMETER_OFFSET>
  {
    // Invoked on no parameters
    static void Invoke(VM *vm, int sp_offset, TypeId type_id, TypeId return_type_id,
                       Estimator const &e, StaticMemberFunction const &f)
    {
      using InvokerType =
          StaticMemberFunctionInvokerHelper<Estimator, ReturnType, StaticMemberFunction>;
      InvokerType::Invoke(vm, sp_offset, type_id, return_type_id, e, f);
    }
  };
};

template <typename Estimator, typename ReturnType, typename... Ts>
void InvokeStaticMemberFunction(VM *vm, TypeId type_id, TypeId return_type_id,
                                ReturnType (*f)(VM *, TypeId, Ts...), Estimator const &e)
{
  constexpr int num_parameters         = int(sizeof...(Ts));
  constexpr int first_parameter_offset = num_parameters - 1;
  constexpr int sp_offset              = num_parameters - IsResult<ReturnType>::value;
  using StaticMemberFunction           = ReturnType (*)(VM *, TypeId, Ts...);
  using StaticMemberFunctionInvoker    = typename StaticMemberFunctionInvoker<
      Estimator, ReturnType, StaticMemberFunction>::template Invoker<first_parameter_offset, Ts...>;
  StaticMemberFunctionInvoker::Invoke(vm, sp_offset, type_id, return_type_id, e, f);
}

}  // namespace vm
}  // namespace fetch
