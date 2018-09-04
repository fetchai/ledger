#pragma once

namespace fetch
{
namespace vm
{
namespace details 
{

template <typename class_type,  typename... used_args>
struct ConstructorMagic
{
  template <int R, typename... remaining_args>
  struct LoopOver;

  template <int R, typename T, typename... remaining_args>
  struct LoopOver<R, T, remaining_args...>
  {
    static class_type Build(VM *vm, used_args &... used)
    {
      typename std::decay<T>::type l =  LoaderClass< typename std::decay<T>::type  >::LoadArgument( R, vm);

      return ConstructorMagic<class_type, used_args..., T>::
          template LoopOver<R-1,  remaining_args...>::Build(
              vm, used..., l);
    }
  };

  template <int R,  typename T>
  struct LoopOver<R, T>
  {
    static class_type Build(VM *vm, used_args &... used)
    {
      typename std::decay<T>::type l = LoaderClass< typename std::decay<T>::type >::LoadArgument(R, vm);

      return class_type(used..., l);
    }
  };

  template <int R>
  struct LoopOver<R>
  {
    static class_type Build(VM *vm, class_type &cls)
    {

      return class_type();
    }

  };
};


}
}
}
