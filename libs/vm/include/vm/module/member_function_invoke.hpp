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

template <typename Estimator, typename Type, typename ReturnType, typename MemberFunction,
          typename... Ts>
struct MemberFunctionInvokerHelper
{
  static void Invoke(VM *vm, int sp_offset, TypeId return_type_id, Estimator const &e,
                     MemberFunction const &f, Ts const &... parameters)
  {
    if (EstimateCharge(vm, {e}, parameters...))
    {
      Variant & v      = vm->stack_[vm->sp_ - sp_offset];
      Ptr<Type> object = v.object;
      if (object)
      {
        ReturnType result((*object.*f)(parameters...));
        StackSetter<ReturnType>::Set(vm, sp_offset, std::move(result), return_type_id);
        vm->sp_ -= sp_offset;
        return;
      }
      vm->RuntimeError("null reference");
    }
  }
};

template <typename Estimator, typename Type, typename MemberFunction, typename... Ts>
struct MemberFunctionInvokerHelper<Estimator, Type, void, MemberFunction, Ts...>
{
  static void Invoke(VM *vm, int sp_offset, TypeId /* return_type_id */, Estimator const &e,
                     MemberFunction const &f, Ts const &... parameters)
  {
    if (EstimateCharge(vm, {e}, parameters...))
    {
      Variant & v      = vm->stack_[vm->sp_ - sp_offset];
      Ptr<Type> object = v.object;
      if (object)
      {
        (*object.*f)(parameters...);
        v.Reset();
        vm->sp_ -= sp_offset + 1;
        return;
      }
      vm->RuntimeError("null reference");
    }
  }
};

template <typename Estimator, typename Type, typename ReturnType, typename MemberFunction,
          typename... Used>
struct MemberFunctionInvoker
{
  template <int PARAMETER_OFFSET, typename... Ts>
  struct Invoker;
  template <int PARAMETER_OFFSET, typename T, typename... Ts>
  struct Invoker<PARAMETER_OFFSET, T, Ts...>
  {
    // Invoked on non-final parameter
    static void Invoke(VM *vm, int sp_offset, TypeId return_type_id, Estimator const &e,
                       MemberFunction const &f, Used const &... used)
    {
      using P = std::decay_t<T>;
      P parameter(StackGetter<P>::Get(vm, PARAMETER_OFFSET));
      using InvokerType =
          typename MemberFunctionInvoker<Estimator, Type, ReturnType, MemberFunction, Used...,
                                         T>::template Invoker<PARAMETER_OFFSET - 1, Ts...>;
      InvokerType::Invoke(vm, sp_offset, return_type_id, e, f, used..., parameter);
    }
  };
  template <int PARAMETER_OFFSET, typename T>
  struct Invoker<PARAMETER_OFFSET, T>
  {
    // Invoked on final parameter
    static void Invoke(VM *vm, int sp_offset, TypeId return_type_id, Estimator const &e,
                       MemberFunction const &f, Used const &... used)
    {
      using P = std::decay_t<T>;
      P parameter(StackGetter<P>::Get(vm, PARAMETER_OFFSET));
      using InvokerType =
          MemberFunctionInvokerHelper<Estimator, Type, ReturnType, MemberFunction, Used..., T>;
      InvokerType::Invoke(vm, sp_offset, return_type_id, e, f, used..., parameter);
    }
  };
  template <int PARAMETER_OFFSET>
  struct Invoker<PARAMETER_OFFSET>
  {
    // Invoked on no parameters
    static void Invoke(VM *vm, int sp_offset, TypeId return_type_id, Estimator const &e,
                       MemberFunction const &f)
    {
      using InvokerType = MemberFunctionInvokerHelper<Estimator, Type, ReturnType, MemberFunction>;
      InvokerType::Invoke(vm, sp_offset, return_type_id, e, f);
    }
  };
};

template <typename Estimator, typename Type, typename ReturnType, typename... Ts>
void InvokeMemberFunction(VM *vm, TypeId return_type_id, ReturnType (Type::*f)(Ts...),
                          Estimator const &e)
{
  constexpr int num_parameters         = int(sizeof...(Ts));
  constexpr int first_parameter_offset = num_parameters - 1;
  constexpr int sp_offset              = num_parameters;
  using MemberFunction                 = ReturnType (Type::*)(Ts...);
  using MemberFunctionInvoker          = typename MemberFunctionInvoker<
      Estimator, Type, ReturnType, MemberFunction>::template Invoker<first_parameter_offset, Ts...>;
  MemberFunctionInvoker::Invoke(vm, sp_offset, return_type_id, e, f);
}

template <typename Estimator, typename Type, typename ReturnType, typename... Ts>
void InvokeMemberFunction(VM *vm, TypeId return_type_id, ReturnType (Type::*f)(Ts...) const,
                          Estimator const &e)
{
  constexpr int num_parameters         = int(sizeof...(Ts));
  constexpr int first_parameter_offset = num_parameters - 1;
  constexpr int sp_offset              = num_parameters;
  using MemberFunction                 = ReturnType (Type::*)(Ts...) const;
  using MemberFunctionInvoker          = typename MemberFunctionInvoker<
      Estimator, Type, ReturnType, MemberFunction>::template Invoker<first_parameter_offset, Ts...>;
  MemberFunctionInvoker::Invoke(vm, sp_offset, return_type_id, e, f);
}

}  // namespace vm
}  // namespace fetch
