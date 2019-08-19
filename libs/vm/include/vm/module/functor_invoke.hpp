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

#include <utility>

namespace fetch {
namespace vm {

template <typename Estimator, typename ReturnType, typename Functor, typename... Args>
struct FunctorInvokerHelper
{
  static void Invoke(VM *vm, int sp_offset, TypeId return_type_id, Functor const &functor,
                     Estimator const &e, Args const &... parameters)
  {
    if (EstimateCharge(vm, {e}, parameters...))
    {
      ReturnType result(functor(vm, parameters...));
      StackSetter<ReturnType>::Set(vm, sp_offset, std::move(result), return_type_id);
      vm->sp_ -= sp_offset;
    }
  };
};

template <typename Estimator, typename Functor, typename... Args>
struct FunctorInvokerHelper<Estimator, void, Functor, Args...>
{
  static void Invoke(VM *vm, int sp_offset, TypeId /* return_type_id */, Functor const &functor,
                     Estimator const &e, Args const &... parameters)
  {
    if (EstimateCharge(vm, {e}, parameters...))
    {
      functor(vm, parameters...);
      vm->sp_ -= sp_offset;
    }
  };
};

template <typename Estimator, typename ReturnType, typename Functor, typename... Used>
struct FunctorInvoker
{
  template <int PARAMETER_OFFSET, typename... Ts>
  struct Invoker;
  template <int PARAMETER_OFFSET, typename T, typename... Ts>
  struct Invoker<PARAMETER_OFFSET, T, Ts...>
  {
    // Invoked on non-final parameter
    static void Invoke(VM *vm, int sp_offset, TypeId return_type_id, Functor const &functor,
                       Estimator const &e, Used const &... used)
    {
      using P = std::decay_t<T>;
      P parameter(StackGetter<P>::Get(vm, PARAMETER_OFFSET));
      using InvokerType = typename FunctorInvoker<Estimator, ReturnType, Functor, Used...,
                                                  T>::template Invoker<PARAMETER_OFFSET - 1, Ts...>;
      InvokerType::Invoke(vm, sp_offset, return_type_id, functor, e, used..., parameter);
    }
  };
  template <int PARAMETER_OFFSET, typename T>
  struct Invoker<PARAMETER_OFFSET, T>
  {
    // Invoked on final parameter
    static void Invoke(VM *vm, int sp_offset, TypeId return_type_id, Functor const &functor,
                       Estimator const &e, Used const &... used)
    {
      using P = std::decay_t<T>;
      P parameter(StackGetter<P>::Get(vm, PARAMETER_OFFSET));
      using InvokerType = FunctorInvokerHelper<Estimator, ReturnType, Functor, Used..., T>;
      InvokerType::Invoke(vm, sp_offset, return_type_id, functor, e, used..., parameter);
    }
  };
  template <int PARAMETER_OFFSET>
  struct Invoker<PARAMETER_OFFSET>
  {
    // Invoked on no parameters
    static void Invoke(VM *vm, int sp_offset, TypeId return_type_id, Functor const &functor,
                       Estimator const &e)
    {
      using InvokerType = FunctorInvokerHelper<Estimator, ReturnType, Functor>;
      InvokerType::Invoke(vm, sp_offset, return_type_id, functor, e);
    }
  };
};

template <typename Estimator, typename Functor, typename... Args>
void InvokeFunctor(VM *vm, TypeId return_type_id, Functor const &functor, Estimator const &e,
                   std::tuple<Args...> && /* tag */)
{
  using ReturnType                     = typename FunctorTraits<Functor>::return_type;
  constexpr int num_parameters         = int(sizeof...(Args));
  constexpr int first_parameter_offset = num_parameters - 1;
  constexpr int sp_offset              = num_parameters - IsResult<ReturnType>::value;
  using FunctorInvoker =
      typename FunctorInvoker<Estimator, ReturnType,
                              Functor>::template Invoker<first_parameter_offset, Args...>;
  FunctorInvoker::Invoke(vm, sp_offset, return_type_id, functor, e);
}

}  // namespace vm
}  // namespace fetch
