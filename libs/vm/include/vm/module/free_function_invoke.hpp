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

template <typename ReturnType, typename FreeFunction, typename... Ts>
struct FreeFunctionInvokerHelper
{
  static void Invoke(VM *vm, int sp_offset, TypeId return_type_id, FreeFunction f,
                     Ts const &... parameters)
  {
    ReturnType result((*f)(vm, parameters...));
    StackSetter<ReturnType>::Set(vm, sp_offset, std::move(result), return_type_id);
    vm->sp_ -= sp_offset;
  };
};

template <typename FreeFunction, typename... Ts>
struct FreeFunctionInvokerHelper<void, FreeFunction, Ts...>
{
  static void Invoke(VM *vm, int sp_offset, TypeId /* return_type_id */, FreeFunction f,
                     Ts const &... parameters)
  {
    (*f)(vm, parameters...);
    vm->sp_ -= sp_offset;
  };
};

template <typename ReturnType, typename FreeFunction, typename... Used>
struct FreeFunctionInvoker
{
  template <int parameter_offset, typename... Ts>
  struct Invoker;
  template <int parameter_offset, typename T, typename... Ts>
  struct Invoker<parameter_offset, T, Ts...>
  {
    // Invoked on non-final parameter
    static void Invoke(VM *vm, int sp_offset, TypeId return_type_id, FreeFunction f,
                       Used const &... used)
    {
      using P = std::decay_t<T>;
      P parameter(StackGetter<P>::Get(vm, parameter_offset));
      using Type = typename FreeFunctionInvoker<ReturnType, FreeFunction, Used...,
                                                T>::template Invoker<parameter_offset - 1, Ts...>;
      Type::Invoke(vm, sp_offset, return_type_id, f, used..., parameter);
    }
  };
  template <int parameter_offset, typename T>
  struct Invoker<parameter_offset, T>
  {
    // Invoked on final parameter
    static void Invoke(VM *vm, int sp_offset, TypeId return_type_id, FreeFunction f,
                       Used const &... used)
    {
      using P = std::decay_t<T>;
      P parameter(StackGetter<P>::Get(vm, parameter_offset));
      using Type = FreeFunctionInvokerHelper<ReturnType, FreeFunction, Used..., T>;
      Type::Invoke(vm, sp_offset, return_type_id, f, used..., parameter);
    }
  };
  template <int parameter_offset>
  struct Invoker<parameter_offset>
  {
    // Invoked on no parameters
    static void Invoke(VM *vm, int sp_offset, TypeId return_type_id, FreeFunction f)
    {
      using Type = FreeFunctionInvokerHelper<ReturnType, FreeFunction>;
      Type::Invoke(vm, sp_offset, return_type_id, f);
    }
  };
};

template <typename ReturnType, typename... Ts>
void InvokeFreeFunction(VM *vm, TypeId return_type_id, ReturnType (*f)(VM *, Ts...))
{
  constexpr int num_parameters         = int(sizeof...(Ts));
  constexpr int first_parameter_offset = num_parameters - 1;
  constexpr int sp_offset              = num_parameters - IsResult<ReturnType>::value;
  using FreeFunction                   = ReturnType (*)(VM *, Ts...);
  using FreeFunctionInvoker =
      typename FreeFunctionInvoker<ReturnType,
                                   FreeFunction>::template Invoker<first_parameter_offset, Ts...>;
  FreeFunctionInvoker::Invoke(vm, sp_offset, return_type_id, f);
}

}  // namespace vm
}  // namespace fetch
