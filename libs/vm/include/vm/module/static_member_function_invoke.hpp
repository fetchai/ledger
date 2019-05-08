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

namespace fetch {
namespace vm {

template <typename ReturnType, typename StaticMemberFunction, typename... Ts>
struct StaticMemberFunctionInvokerHelper
{
  static void Invoke(VM *vm, int sp_offset, TypeId type_id, TypeId return_type_id, StaticMemberFunction f,
                     Ts const &... parameters)
  {
    ReturnType result((*f)(vm, type_id, parameters...));
    StackSetter<ReturnType>::Set(vm, sp_offset, std::move(result), return_type_id);
    vm->sp_ -= sp_offset;
  };
};

template <typename StaticMemberFunction, typename... Ts>
struct StaticMemberFunctionInvokerHelper<void, StaticMemberFunction, Ts...>
{
  static void Invoke(VM *vm, int sp_offset, TypeId type_id, TypeId /* return_type_id */,
                     StaticMemberFunction f, Ts const &... parameters)
  {
    (*f)(vm, type_id, parameters...);
    vm->sp_ -= sp_offset;
  };
};

template <typename ReturnType, typename StaticMemberFunction, typename... Used>
struct StaticMemberFunctionInvoker
{
  template <int PARAMETER_OFFSET, typename... Ts>
  struct Invoker;
  template <int PARAMETER_OFFSET, typename T, typename... Ts>
  struct Invoker<PARAMETER_OFFSET, T, Ts...>
  {
    // Invoked on non-final parameter
    static void Invoke(VM *vm, int sp_offset, TypeId type_id, TypeId return_type_id, StaticMemberFunction f,
                       Used const &... used)
    {
      using P = std::decay_t<T>;
      P parameter(StackGetter<P>::Get(vm, PARAMETER_OFFSET));
      using InvokerType = typename StaticMemberFunctionInvoker<ReturnType, StaticMemberFunction, Used...,
                                                T>::template Invoker<PARAMETER_OFFSET - 1, Ts...>;
      InvokerType::Invoke(vm, sp_offset, type_id, return_type_id, f, used..., parameter);
    }
  };
  template <int PARAMETER_OFFSET, typename T>
  struct Invoker<PARAMETER_OFFSET, T>
  {
    // Invoked on final parameter
    static void Invoke(VM *vm, int sp_offset, TypeId type_id, TypeId return_type_id, StaticMemberFunction f,
                       Used const &... used)
    {
      using P = std::decay_t<T>;
      P parameter(StackGetter<P>::Get(vm, PARAMETER_OFFSET));
      using InvokerType = StaticMemberFunctionInvokerHelper<ReturnType, StaticMemberFunction, Used..., T>;
      InvokerType::Invoke(vm, sp_offset, type_id, return_type_id, f, used..., parameter);
    }
  };
  template <int PARAMETER_OFFSET>
  struct Invoker<PARAMETER_OFFSET>
  {
    // Invoked on no parameters
    static void Invoke(VM *vm, int sp_offset, TypeId type_id, TypeId return_type_id, StaticMemberFunction f)
    {
      using InvokerType = StaticMemberFunctionInvokerHelper<ReturnType, StaticMemberFunction>;
      InvokerType::Invoke(vm, sp_offset, type_id, return_type_id, f);
    }
  };
};

template <typename ReturnType, typename... Ts>
void InvokeStaticMemberFunction(VM *vm, TypeId type_id, TypeId return_type_id,
                        ReturnType (*f)(VM *, TypeId, Ts...))
{
  constexpr int num_parameters         = int(sizeof...(Ts));
  constexpr int first_parameter_offset = num_parameters - 1;
  constexpr int sp_offset              = num_parameters - IsResult<ReturnType>::value;
  using StaticMemberFunction           = ReturnType (*)(VM *, TypeId, Ts...);
  using StaticMemberFunctionInvoker =
      typename StaticMemberFunctionInvoker<ReturnType,
                                   StaticMemberFunction>::template Invoker<first_parameter_offset, Ts...>;
  StaticMemberFunctionInvoker::Invoke(vm, sp_offset, type_id, return_type_id, f);
}

}  // namespace vm
}  // namespace fetch
