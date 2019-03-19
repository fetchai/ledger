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

template <typename ObjectType, typename ReturnType, typename InstanceFunction, typename... Ts>
struct InstanceFunctionInvokerHelper
{
  static void Invoke(VM *vm, int sp_offset, TypeId return_type_id, InstanceFunction f,
                     Ts const &... parameters)
  {
    Variant &       v      = vm->stack_[vm->sp_ - sp_offset];
    Ptr<ObjectType> object = v.object;
    if (object)
    {
      ReturnType result((*object.*f)(parameters...));
      StackSetter<ReturnType>::Set(vm, sp_offset, std::move(result), return_type_id);
      vm->sp_ -= sp_offset;
      return;
    }
    vm->RuntimeError("null reference");
  };
};

template <typename ObjectType, typename InstanceFunction, typename... Ts>
struct InstanceFunctionInvokerHelper<ObjectType, void, InstanceFunction, Ts...>
{
  static void Invoke(VM *vm, int sp_offset, TypeId /* return_type_id */, InstanceFunction f,
                     Ts const &... parameters)
  {
    Variant &       v      = vm->stack_[vm->sp_ - sp_offset];
    Ptr<ObjectType> object = v.object;
    if (object)
    {
      (*object.*f)(parameters...);
      v.Reset();
      vm->sp_ -= sp_offset + 1;
      return;
    }
    vm->RuntimeError("null reference");
  }
};

template <typename ObjectType, typename ReturnType, typename InstanceFunction, typename... Used>
struct InstanceFunctionInvoker
{
  template <int parameter_offset, typename... Ts>
  struct Invoker;
  template <int parameter_offset, typename T, typename... Ts>
  struct Invoker<parameter_offset, T, Ts...>
  {
    // Invoked on non-final parameter
    static void Invoke(VM *vm, int sp_offset, TypeId return_type_id, InstanceFunction f,
                       Used const &... used)
    {
      using P = std::decay_t<T>;
      P parameter(StackGetter<P>::Get(vm, parameter_offset));
      using Type =
          typename InstanceFunctionInvoker<ObjectType, ReturnType, InstanceFunction, Used...,
                                           T>::template Invoker<parameter_offset - 1, Ts...>;
      Type::Invoke(vm, sp_offset, return_type_id, f, used..., parameter);
    }
  };
  template <int parameter_offset, typename T>
  struct Invoker<parameter_offset, T>
  {
    // Invoked on final parameter
    static void Invoke(VM *vm, int sp_offset, TypeId return_type_id, InstanceFunction f,
                       Used const &... used)
    {
      using P = std::decay_t<T>;
      P parameter(StackGetter<P>::Get(vm, parameter_offset));
      using Type =
          InstanceFunctionInvokerHelper<ObjectType, ReturnType, InstanceFunction, Used..., T>;
      Type::Invoke(vm, sp_offset, return_type_id, f, used..., parameter);
    }
  };
  template <int parameter_offset>
  struct Invoker<parameter_offset>
  {
    // Invoked on no parameters
    static void Invoke(VM *vm, int sp_offset, TypeId return_type_id, InstanceFunction f)
    {
      using Type = InstanceFunctionInvokerHelper<ObjectType, ReturnType, InstanceFunction>;
      Type::Invoke(vm, sp_offset, return_type_id, f);
    }
  };
};

template <typename ObjectType, typename ReturnType, typename... Ts>
void InvokeInstanceFunction(VM *vm, TypeId return_type_id, ReturnType (ObjectType::*f)(Ts...))
{
  constexpr int num_parameters         = int(sizeof...(Ts));
  constexpr int first_parameter_offset = num_parameters - 1;
  constexpr int sp_offset              = num_parameters;
  using InstanceFunction               = ReturnType (ObjectType::*)(Ts...);
  using InstanceFunctionInvoker        = typename InstanceFunctionInvoker<
      ObjectType, ReturnType, InstanceFunction>::template Invoker<first_parameter_offset, Ts...>;
  InstanceFunctionInvoker::Invoke(vm, sp_offset, return_type_id, f);
}

template <typename ObjectType, typename ReturnType, typename... Ts>
void InvokeInstanceFunction(VM *vm, TypeId return_type_id, ReturnType (ObjectType::*f)(Ts...) const)
{
  constexpr int num_parameters         = int(sizeof...(Ts));
  constexpr int first_parameter_offset = num_parameters - 1;
  constexpr int sp_offset              = num_parameters;
  using InstanceFunction               = ReturnType (ObjectType::*)(Ts...) const;
  using InstanceFunctionInvoker        = typename InstanceFunctionInvoker<
      ObjectType, ReturnType, InstanceFunction>::template Invoker<first_parameter_offset, Ts...>;
  InstanceFunctionInvoker::Invoke(vm, sp_offset, return_type_id, f);
}

}  // namespace vm
}  // namespace fetch
