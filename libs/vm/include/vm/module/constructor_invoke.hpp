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

#include "vm/module/base.hpp"
#include "vm/object.hpp"
#include "vm/vm.hpp"

namespace fetch {
namespace vm {

template <typename Type, typename ReturnType, typename Constructor, typename... Ts>
struct ConstructorInvokerHelper
{
  static void Invoke(VM *vm, int sp_offset, TypeId type_id, Constructor const &constructor,
                     Ts const &... parameters)
  {
    ReturnType result(constructor(vm, type_id, parameters...));
    StackSetter<ReturnType>::Set(vm, sp_offset, std::move(result), type_id);
    vm->sp_ -= sp_offset;
  };
};

template <typename Type, typename ReturnType, typename Constructor, typename... Used>
struct ConstructorInvoker
{
  template <int PARAMETER_OFFSET, typename... Ts>
  struct Invoker;
  template <int PARAMETER_OFFSET, typename T, typename... Ts>
  struct Invoker<PARAMETER_OFFSET, T, Ts...>
  {
    // Invoked on non-final parameter
    static void Invoke(VM *vm, int sp_offset, TypeId type_id, Constructor const &constructor,
                       Used const &... used)
    {
      using P = std::decay_t<T>;
      P parameter(StackGetter<P>::Get(vm, PARAMETER_OFFSET));
      using InvokerType =
          typename ConstructorInvoker<Type, ReturnType, Constructor, Used...,
                                      T>::template Invoker<PARAMETER_OFFSET - 1, Ts...>;
      InvokerType::Invoke(vm, sp_offset, type_id, constructor, used..., parameter);
    }
  };
  template <int PARAMETER_OFFSET, typename T>
  struct Invoker<PARAMETER_OFFSET, T>
  {
    // Invoked on final parameter
    static void Invoke(VM *vm, int sp_offset, TypeId type_id, Constructor const &constructor,
                       Used const &... used)
    {
      using P = std::decay_t<T>;
      P parameter(StackGetter<P>::Get(vm, PARAMETER_OFFSET));
      using InvokerType = ConstructorInvokerHelper<Type, ReturnType, Constructor, Used..., T>;
      InvokerType::Invoke(vm, sp_offset, type_id, constructor, used..., parameter);
    }
  };
  template <int PARAMETER_OFFSET>
  struct Invoker<PARAMETER_OFFSET>
  {
    // Invoked on no parameters
    static void Invoke(VM *vm, int sp_offset, TypeId type_id, Constructor const &constructor)
    {
      using InvokerType = ConstructorInvokerHelper<Type, ReturnType, Constructor>;
      InvokerType::Invoke(vm, sp_offset, type_id, constructor);
    }
  };
};

template <typename ReturnType, typename... Args>
void InvokeConstructor(VM *vm, TypeId type_id, ReturnType (*constructor)(VM *, TypeId, Args...))
{
  static_assert(IsPtr<ReturnType>::value, "Constructors must return a fetch::vm::Ptr");

  constexpr int num_parameters         = int(sizeof...(Args));
  constexpr int first_parameter_offset = num_parameters - 1;
  constexpr int sp_offset              = first_parameter_offset;

  using ConstructorInvoker =
      typename ConstructorInvoker<TypeFromPtr<ReturnType>, ReturnType, decltype(constructor)>::
          template Invoker<first_parameter_offset, Args...>;
  ConstructorInvoker::Invoke(vm, sp_offset, type_id, constructor);
}

}  // namespace vm
}  // namespace fetch
