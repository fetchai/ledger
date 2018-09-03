#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018 Fetch.AI Limited
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

#include "stack_loader.hpp"

namespace fetch {
namespace vm {
namespace details {

template <typename FunctionPointer, typename ReturnType, int RESULT_POSITION,
          typename... UsedArgs>
struct InvokeStaticOrFreeFunction
{
  static void StaticOrFreeFunction(VM *vm, FunctionPointer &m, UsedArgs &... args)
  {
    ReturnType ret = (*m)(args...);
    StorerClass<ReturnType, RESULT_POSITION>::StoreArgument(vm, std::move(ret));
  };
};

template <typename FunctionPointer, int RESULT_POSITION, typename... UsedArgs>
struct InvokeStaticOrFreeFunction<FunctionPointer, void, RESULT_POSITION, UsedArgs...>
{
  static void StaticOrFreeFunction(VM *vm, FunctionPointer &m, UsedArgs &... args)
  {
    (*m)(args...);
  };
};

template <typename FunctionPointer, typename ReturnType, std::size_t RESULT_POSITION,
          typename... UsedArgs>
struct StaticOrFreeFunctionMagic
{
  template <int R, typename... RemainingArgs>
  struct LoopOver;

  template <int R, typename T, typename... RemainingArgs>
  struct LoopOver<R, T, RemainingArgs...>
  {
    static void Apply(VM *vm, FunctionPointer &m, UsedArgs &... used)
    {
      typename std::decay<T>::type l =
          LoaderClass<typename std::decay<T>::type>::LoadArgument(R, vm);

      StaticOrFreeFunctionMagic<FunctionPointer, ReturnType, RESULT_POSITION, UsedArgs...,
                                T>::template LoopOver<R - 1, RemainingArgs...>::Apply(vm, m,
                                                                                       used..., l);
    }
  };

  template <int R, typename T>
  struct LoopOver<R, T>
  {
    static void Apply(VM *vm, FunctionPointer &m, UsedArgs &... used)
    {
      typename std::decay<T>::type l =
          LoaderClass<typename std::decay<T>::type>::LoadArgument(R, vm);

      InvokeStaticOrFreeFunction<FunctionPointer, ReturnType, RESULT_POSITION, UsedArgs...,
                                 T>::StaticOrFreeFunction(vm, m, used..., l);
    }
  };

  template <int R>
  struct LoopOver<R>
  {
    static void Apply(VM *vm, FunctionPointer &m)
    {
      InvokeStaticOrFreeFunction<FunctionPointer, ReturnType,
                                 RESULT_POSITION>::StaticOrFreeFunction(vm, m);
    }
  };
};

}  // namespace details

}  // namespace vm
}  // namespace fetch
