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

template <typename ClassType, typename... UsedArgs>
struct ConstructorMagic
{
  template <int R, typename... RemainingArgs>
  struct LoopOver;

  template <int R, typename T, typename... RemainingArgs>
  struct LoopOver<R, T, RemainingArgs...>
  {
    static ClassType Build(VM *vm, UsedArgs &... used)
    {
      typename std::decay<T>::type l =
          LoaderClass<typename std::decay<T>::type>::LoadArgument(R, vm);

      return ConstructorMagic<ClassType, UsedArgs...,
                              T>::template LoopOver<R - 1, RemainingArgs...>::Build(vm, used..., l);
    }
  };

  template <int R, typename T>
  struct LoopOver<R, T>
  {
    static ClassType Build(VM *vm, UsedArgs &... used)
    {
      typename std::decay<T>::type l =
          LoaderClass<typename std::decay<T>::type>::LoadArgument(R, vm);

      return ClassType(used..., l);
    }
  };

  template <int R>
  struct LoopOver<R>
  {
    static ClassType Build(VM *vm, ClassType &cls) { return ClassType(); }
  };
};

}  // namespace details
}  // namespace vm
}  // namespace fetch
