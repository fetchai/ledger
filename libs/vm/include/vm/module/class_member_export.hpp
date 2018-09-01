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

namespace fetch {
namespace vm {
namespace details {

template <typename class_type, typename member_function_pointer, typename return_type,
          int result_position, typename... used_args>
struct InvokeClassMemberFunction
{
  static void MemberFunction(VM *vm, class_type &cls, member_function_pointer &m,
                             used_args &... args)
  {
    auto ret = (cls.*m)(args...);
    StorerClass<return_type, result_position>::StoreArgument(vm, std::move(ret));
  };
};

template <typename class_type, typename member_function_pointer, int result_position,
          typename... used_args>
struct InvokeClassMemberFunction<class_type, member_function_pointer, void, result_position,
                                 used_args...>
{
  static void MemberFunction(VM *vm, class_type &cls, member_function_pointer &m,
                             used_args &... args)
  {
    (cls.*m)(args...);
  };
};

template <typename class_type, typename member_function_pointer, typename return_type,
          int result_position, typename... used_args>
struct MemberFunctionMagic
{
  template <int R, typename... remaining_args>
  struct LoopOver;

  template <int R, typename T, typename... remaining_args>
  struct LoopOver<R, T, remaining_args...>
  {
    static void Apply(VM *vm, class_type &cls, member_function_pointer &m, used_args &... used)
    {
      typename std::decay<T>::type l =
          LoaderClass<typename std::decay<T>::type>::LoadArgument(R, vm);

      MemberFunctionMagic<class_type, member_function_pointer, return_type, result_position,
                          used_args...,
                          T>::template LoopOver<R - 1, remaining_args...>::Apply(vm, cls, m,
                                                                                 used..., l);
    }
  };

  template <int R, typename T>
  struct LoopOver<R, T>
  {
    static void Apply(VM *vm, class_type &cls, member_function_pointer &m, used_args &... used)
    {
      typename std::decay<T>::type l =
          LoaderClass<typename std::decay<T>::type>::LoadArgument(R, vm);

      InvokeClassMemberFunction<class_type, member_function_pointer, return_type, result_position,
                                used_args..., T>::MemberFunction(vm, cls, m, used..., l);
    }
  };

  template <int R>
  struct LoopOver<R>
  {
    static void Apply(VM *vm, class_type &cls, member_function_pointer &m)
    {
      InvokeClassMemberFunction<class_type, member_function_pointer, return_type,
                                result_position>::MemberFunction(vm, cls, m);
    }
  };
};

}  // namespace details

}  // namespace vm
}  // namespace fetch
