#pragma once

#include "stack_loader.hpp"

namespace fetch
{
namespace vm
{
namespace details 
{

template <typename function_pointer, typename return_type, int result_position, typename... used_args>
struct InvokeStaticOrFreeFunction
{
  static void StaticOrFreeFunction(VM * vm, function_pointer &m,
                             used_args &... args)
  {
    return_type                                      ret = (*m)(args...);
    StorerClass< return_type, result_position >::StoreArgument(vm, std::move(ret));
  };
};

template <typename function_pointer, int result_position, typename... used_args>
struct InvokeStaticOrFreeFunction<function_pointer, void, result_position, used_args...>
{
  static void StaticOrFreeFunction(VM * vm, function_pointer &m,
                             used_args &... args)
  {
    (*m)(args...);
  };
};


template <typename function_pointer, typename return_type,
          std::size_t result_position,
          typename... used_args>
struct StaticOrFreeFunctionMagic
{
  template <int R, typename... remaining_args>
  struct LoopOver;

  template <int R, typename T, typename... remaining_args>
  struct LoopOver<R, T, remaining_args...>
  {
    static void Apply(VM *vm, function_pointer &m,
                       used_args &... used)
    {
      typename std::decay<T>::type l =  LoaderClass< typename std::decay<T>::type  >::LoadArgument( R, vm);

      StaticOrFreeFunctionMagic<function_pointer, return_type, result_position, used_args..., T>::
          template LoopOver<R-1,  remaining_args...>::Apply(
              vm,  m,  used..., l);
    }
  };

  template <int R,  typename T>
  struct LoopOver<R, T>
  {
    static void Apply(VM *vm, function_pointer &m,
                       used_args &... used)
    {
      typename std::decay<T>::type l = LoaderClass< typename std::decay<T>::type >::LoadArgument(R, vm);

      InvokeStaticOrFreeFunction<function_pointer, return_type, result_position, used_args..., T>::StaticOrFreeFunction(vm, m,  used..., l);
    }
  };

  template <int R>
  struct LoopOver<R>
  {
    static void Apply(VM *vm, function_pointer &m)
    {
      InvokeStaticOrFreeFunction<function_pointer, return_type, result_position>::StaticOrFreeFunction(vm, m);
    }

  };
};

}

}
}