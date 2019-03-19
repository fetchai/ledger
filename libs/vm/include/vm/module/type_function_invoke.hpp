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

template <typename ReturnType, typename TypeFunction, typename... Ts>
struct TypeFunctionInvokerHelper
{
  static void Invoke(VM *vm, int sp_offset, TypeId type_id, TypeId return_type_id, TypeFunction f,
                     Ts const &... parameters)
  {
    ReturnType result((*f)(vm, type_id, parameters...));
    StackSetter<ReturnType>::Set(vm, sp_offset, std::move(result), return_type_id);
    vm->sp_ -= sp_offset;
  };
};

template <typename TypeFunction, typename... Ts>
struct TypeFunctionInvokerHelper<void, TypeFunction, Ts...>
{
  static void Invoke(VM *vm, int sp_offset, TypeId type_id, TypeId /* return_type_id */,
                     TypeFunction f, Ts const &... parameters)
  {
    (*f)(vm, type_id, parameters...);
    vm->sp_ -= sp_offset;
  };
};

template <typename ReturnType, typename TypeFunction, typename... Used>
struct TypeFunctionInvoker
{
  template <int parameter_offset, typename... Ts>
  struct Invoker;
  template <int parameter_offset, typename T, typename... Ts>
  struct Invoker<parameter_offset, T, Ts...>
  {
    // Invoked on non-final parameter
    static void Invoke(VM *vm, int sp_offset, TypeId type_id, TypeId return_type_id, TypeFunction f,
                       Used const &... used)
    {
      using P = std::decay_t<T>;
      P parameter(StackGetter<P>::Get(vm, parameter_offset));
      using Type = typename TypeFunctionInvoker<ReturnType, TypeFunction, Used...,
                                                T>::template Invoker<parameter_offset - 1, Ts...>;
      Type::Invoke(vm, sp_offset, type_id, return_type_id, f, used..., parameter);
    }
  };
  template <int parameter_offset, typename T>
  struct Invoker<parameter_offset, T>
  {
    // Invoked on final parameter
    static void Invoke(VM *vm, int sp_offset, TypeId type_id, TypeId return_type_id, TypeFunction f,
                       Used const &... used)
    {
      using P = std::decay_t<T>;
      P parameter(StackGetter<P>::Get(vm, parameter_offset));
      using Type = TypeFunctionInvokerHelper<ReturnType, TypeFunction, Used..., T>;
      Type::Invoke(vm, sp_offset, type_id, return_type_id, f, used..., parameter);
    }
  };
  template <int parameter_offset>
  struct Invoker<parameter_offset>
  {
    // Invoked on no parameters
    static void Invoke(VM *vm, int sp_offset, TypeId type_id, TypeId return_type_id, TypeFunction f)
    {
      using Type = TypeFunctionInvokerHelper<ReturnType, TypeFunction>;
      Type::Invoke(vm, sp_offset, type_id, return_type_id, f);
    }
  };
};

template <typename ReturnType, typename... Ts>
void InvokeTypeFunction(VM *vm, TypeId type_id, TypeId return_type_id,
                        ReturnType (*f)(VM *, TypeId, Ts...))
{
  constexpr int num_parameters         = int(sizeof...(Ts));
  constexpr int first_parameter_offset = num_parameters - 1;
  constexpr int sp_offset              = num_parameters - IsResult<ReturnType>::value;
  using TypeFunction                   = ReturnType (*)(VM *, TypeId, Ts...);
  using TypeFunctionInvoker =
      typename TypeFunctionInvoker<ReturnType,
                                   TypeFunction>::template Invoker<first_parameter_offset, Ts...>;
  TypeFunctionInvoker::Invoke(vm, sp_offset, type_id, return_type_id, f);
}

}  // namespace vm
}  // namespace fetch
