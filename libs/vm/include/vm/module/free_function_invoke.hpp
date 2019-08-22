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

template <typename Estimator, typename ReturnType, typename FreeFunction, typename... Args>
struct FreeFunctionInvokerHelper
{
  static void Invoke(VM *vm, int sp_offset, TypeId return_type_id, FreeFunction const &f,
                     Estimator const &e, Args const &... parameters)
  {
    if (EstimateCharge(vm, {e}, parameters...))
    {
      ReturnType result((*f)(vm, parameters...));
      StackSetter<ReturnType>::Set(vm, sp_offset, std::move(result), return_type_id);
      vm->sp_ -= sp_offset;
    }
  };
};

template <typename Estimator, typename FreeFunction, typename... Args>
struct FreeFunctionInvokerHelper<Estimator, void, FreeFunction, Args...>
{
  static void Invoke(VM *vm, int sp_offset, TypeId /* return_type_id */, FreeFunction const &f,
                     Estimator const &e, Args const &... parameters)
  {
    if (EstimateCharge(vm, {e}, parameters...))
    {
      (*f)(vm, parameters...);
      vm->sp_ -= sp_offset;
    }
  };
};

template <typename Estimator, typename ReturnType, typename FreeFunction, typename... Used>
struct FreeFunctionInvoker
{
  template <int PARAMETER_OFFSET, typename... Ts>
  struct Invoker;
  template <int PARAMETER_OFFSET, typename T, typename... Ts>
  struct Invoker<PARAMETER_OFFSET, T, Ts...>
  {
    // Invoked on non-final parameter
    static void Invoke(VM *vm, int sp_offset, TypeId return_type_id, FreeFunction const &f,
                       Estimator const &e, Used const &... used)
    {
      using P = std::decay_t<T>;
      P parameter(StackGetter<P>::Get(vm, PARAMETER_OFFSET));
      using InvokerType =
          typename FreeFunctionInvoker<Estimator, ReturnType, FreeFunction, Used...,
                                       T>::template Invoker<PARAMETER_OFFSET - 1, Ts...>;
      InvokerType::Invoke(vm, sp_offset, return_type_id, f, e, used..., parameter);
    }
  };
  template <int PARAMETER_OFFSET, typename T>
  struct Invoker<PARAMETER_OFFSET, T>
  {
    // Invoked on final parameter
    static void Invoke(VM *vm, int sp_offset, TypeId return_type_id, FreeFunction const &f,
                       Estimator const &e, Used const &... used)
    {
      using P = std::decay_t<T>;
      P parameter(StackGetter<P>::Get(vm, PARAMETER_OFFSET));
      using InvokerType =
          FreeFunctionInvokerHelper<Estimator, ReturnType, FreeFunction, Used..., T>;
      InvokerType::Invoke(vm, sp_offset, return_type_id, f, e, used..., parameter);
    }
  };
  template <int PARAMETER_OFFSET>
  struct Invoker<PARAMETER_OFFSET>
  {
    // Invoked on no parameters
    static void Invoke(VM *vm, int sp_offset, TypeId return_type_id, FreeFunction const &f,
                       Estimator const &e)
    {
      using InvokerType = FreeFunctionInvokerHelper<Estimator, ReturnType, FreeFunction>;
      InvokerType::Invoke(vm, sp_offset, return_type_id, f, e);
    }
  };
};

template <typename Estimator, typename ReturnType, typename... Args>
void InvokeFreeFunction(VM *vm, TypeId return_type_id, ReturnType (*f)(VM *, Args...),
                        Estimator const &e)
{
  constexpr int num_parameters         = int(sizeof...(Args));
  constexpr int first_parameter_offset = num_parameters - 1;
  constexpr int sp_offset              = num_parameters - IsResult<ReturnType>::value;
  using FreeFunction                   = ReturnType (*)(VM *, Args...);
  using FreeFunctionInvoker =
      typename FreeFunctionInvoker<Estimator, ReturnType,
                                   FreeFunction>::template Invoker<first_parameter_offset, Args...>;
  FreeFunctionInvoker::Invoke(vm, sp_offset, return_type_id, f, e);
}

}  // namespace vm
}  // namespace fetch
