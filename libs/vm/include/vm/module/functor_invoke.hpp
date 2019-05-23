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

template <typename ReturnType, typename Functor, typename... Ts>
struct FunctorInvokerHelper
{
  static void Invoke(VM *vm, int sp_offset, TypeId return_type_id, Functor const &functor,
                     Ts const &... parameters)
  {
    ReturnType result(functor(vm, parameters...));
    StackSetter<ReturnType>::Set(vm, sp_offset, std::move(result), return_type_id);
    vm->sp_ -= sp_offset;
  };
};

template <typename Functor, typename... Ts>
struct FunctorInvokerHelper<void, Functor, Ts...>
{
  static void Invoke(VM *vm, int sp_offset, TypeId /* return_type_id */, Functor const &functor,
                     Ts const &... parameters)
  {
    functor(vm, parameters...);
    vm->sp_ -= sp_offset;
  };
};

template <typename ReturnType, typename Functor, typename... Used>
struct FunctorInvoker
{
  template <int PARAMETER_OFFSET, typename... Ts>
  struct Invoker;
  template <int PARAMETER_OFFSET, typename T, typename... Ts>
  struct Invoker<PARAMETER_OFFSET, T, Ts...>
  {
    // Invoked on non-final parameter
    static void Invoke(VM *vm, int sp_offset, TypeId return_type_id, Functor const &functor,
                       Used const &... used)
    {
      using P = std::decay_t<T>;
      P parameter(StackGetter<P>::Get(vm, PARAMETER_OFFSET));
      using InvokerType = typename FunctorInvoker<ReturnType, Functor, Used...,
                                                  T>::template Invoker<PARAMETER_OFFSET - 1, Ts...>;
      InvokerType::Invoke(vm, sp_offset, return_type_id, functor, used..., parameter);
    }
  };
  template <int PARAMETER_OFFSET, typename T>
  struct Invoker<PARAMETER_OFFSET, T>
  {
    // Invoked on final parameter
    static void Invoke(VM *vm, int sp_offset, TypeId return_type_id, Functor const &functor,
                       Used const &... used)
    {
      using P = std::decay_t<T>;
      P parameter(StackGetter<P>::Get(vm, PARAMETER_OFFSET));
      using InvokerType = FunctorInvokerHelper<ReturnType, Functor, Used..., T>;
      InvokerType::Invoke(vm, sp_offset, return_type_id, functor, used..., parameter);
    }
  };
  template <int PARAMETER_OFFSET>
  struct Invoker<PARAMETER_OFFSET>
  {
    // Invoked on no parameters
    static void Invoke(VM *vm, int sp_offset, TypeId return_type_id, Functor const &functor)
    {
      using InvokerType = FunctorInvokerHelper<ReturnType, Functor>;
      InvokerType::Invoke(vm, sp_offset, return_type_id, functor);
    }
  };
};

template <typename Functor, typename... Ts>
void InvokeFunctor(VM *vm, TypeId return_type_id, Functor const &functor,
                   std::tuple<Ts...> const & /* tag */)
{
  using ReturnType                     = typename FunctorReturnTypeExtractor<Functor>::type;
  constexpr int num_parameters         = int(sizeof...(Ts));
  constexpr int first_parameter_offset = num_parameters - 1;
  constexpr int sp_offset              = num_parameters - IsResult<ReturnType>::value;
  using FunctorInvoker =
      typename FunctorInvoker<ReturnType, Functor>::template Invoker<first_parameter_offset, Ts...>;
  FunctorInvoker::Invoke(vm, sp_offset, return_type_id, functor);
}

}  // namespace vm
}  // namespace fetch
