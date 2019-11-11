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

#include "meta/callable/callable_traits.hpp"
#include "meta/tuple.hpp"
#include "vm/module/base.hpp"
#include "vm/object.hpp"
#include "vm/vm.hpp"

#include <type_traits>
#include <utility>

namespace fetch {
namespace vm {

template <int sp_offset, typename ReturnType, typename Callable, typename ArgsTuple>
struct VmMemberFunctionInvoker
{
  static void Invoke(VM *vm, ArgsTuple &&arguments, Callable &&callable)
  {
    using OwningType  = typename meta::CallableTraits<Callable>::OwningType;
    auto const offset = sp_offset + 1;

    Variant &       v      = vm->stack_[vm->sp_ - offset];
    Ptr<OwningType> object = std::move(v.object);
    if (object)
    {
      ReturnType result         = meta::Apply(std::forward<Callable>(callable), *object,
                                      std::forward<ArgsTuple>(arguments));
      auto const return_type_id = vm->instruction_->type_id;
      StackSetter<ReturnType>::Set(vm, offset, std::move(result), return_type_id);
      vm->sp_ -= offset;
      return;
    }
    vm->RuntimeError("null reference");
  }
};

template <int sp_offset, typename Callable, typename ArgsTuple>
struct VmMemberFunctionInvoker<sp_offset, void, Callable, ArgsTuple>
{
  static void Invoke(VM *vm, ArgsTuple &&arguments, Callable &&callable)
  {
    using OwningType  = typename meta::CallableTraits<Callable>::OwningType;
    auto const offset = sp_offset + 1;

    Variant &       v      = vm->stack_[vm->sp_ - offset + 1];
    Ptr<OwningType> object = std::move(v.object);
    if (object)
    {
      meta::Apply(std::forward<Callable>(callable), *object, std::forward<ArgsTuple>(arguments));
      v.Reset();
      vm->sp_ -= offset;
      return;
    }
    vm->RuntimeError("null reference");
  }
};

}  // namespace vm
}  // namespace fetch
