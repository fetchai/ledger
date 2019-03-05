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

template <typename ObjectType, typename ReturnType, typename TypeConstructor, typename... Ts>
struct TypeConstructorInvokerHelper
{
  static void Invoke(VM *vm, int sp_offset, TypeId type_id, Ts const &... parameters)
  {
    ReturnType result(ObjectType::Constructor(vm, type_id, parameters...));
    StackSetter<ReturnType>::Set(vm, sp_offset, std::move(result), type_id);
    vm->sp_ -= sp_offset;
  };
};

template <typename ObjectType, typename ReturnType, typename TypeConstructor, typename... Used>
struct TypeConstructorInvoker
{
  template <int parameter_offset, typename... Ts>
  struct Invoker;
  template <int parameter_offset, typename T, typename... Ts>
  struct Invoker<parameter_offset, T, Ts...>
  {
    // Invoked on non-final parameter
    static void Invoke(VM *vm, int sp_offset, TypeId type_id, Used const &... used)
    {
      using P = std::decay_t<T>;
      P parameter(StackGetter<P>::Get(vm, parameter_offset));
      using Type =
          typename TypeConstructorInvoker<ObjectType, ReturnType, TypeConstructor, Used...,
                                          T>::template Invoker<parameter_offset - 1, Ts...>;
      Type::Invoke(vm, sp_offset, type_id, used..., parameter);
    }
  };
  template <int parameter_offset, typename T>
  struct Invoker<parameter_offset, T>
  {
    // Invoked on final parameter
    static void Invoke(VM *vm, int sp_offset, TypeId type_id, Used const &... used)
    {
      using P = std::decay_t<T>;
      P parameter(StackGetter<P>::Get(vm, parameter_offset));
      using Type =
          TypeConstructorInvokerHelper<ObjectType, ReturnType, TypeConstructor, Used..., T>;
      Type::Invoke(vm, sp_offset, type_id, used..., parameter);
    }
  };
  template <int parameter_offset>
  struct Invoker<parameter_offset>
  {
    // Invoked on no parameters
    static void Invoke(VM *vm, int sp_offset, TypeId type_id)
    {
      using Type = TypeConstructorInvokerHelper<ObjectType, ReturnType, TypeConstructor>;
      Type::Invoke(vm, sp_offset, type_id);
    }
  };
};

template <typename ObjectType, typename... Ts>
void InvokeTypeConstructor(VM *vm, TypeId type_id)
{
  constexpr int num_parameters         = int(sizeof...(Ts));
  constexpr int first_parameter_offset = num_parameters - 1;
  constexpr int sp_offset              = first_parameter_offset;
  using ReturnType                     = Ptr<ObjectType>;
  using TypeConstructor                = ReturnType (*)(VM *, TypeId, Ts...);
  using TypeConstructorInvoker         = typename TypeConstructorInvoker<
      ObjectType, ReturnType, TypeConstructor>::template Invoker<first_parameter_offset, Ts...>;
  TypeConstructorInvoker::Invoke(vm, sp_offset, type_id);
}

}  // namespace vm
}  // namespace fetch
