#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018-2020 Fetch.AI Limited
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

template <int sp_offset, typename ReturnType, typename Callable, typename EtchArgsTuple>
struct VmFreeFunctionInvoker
{
  template <typename Estimator, typename... ExtraArgs>
  static void Invoke(VM *vm, Estimator &&estimator, Callable &&callable,
                     EtchArgsTuple &&etch_arguments, ExtraArgs const &... extra_args)
  {
    auto estimator_args_tuple = etch_arguments;
    if (EstimateCharge(vm, std::forward<Estimator>(estimator), std::move(estimator_args_tuple)))
    {
      // prepend extra non-etch arguments (e.g. VM*, TypeId)
      auto args_tuple = std::tuple_cat(std::make_tuple(extra_args...),
                                       std::forward<EtchArgsTuple>(etch_arguments));

      ReturnType result = meta::Apply(std::forward<Callable>(callable), std::move(args_tuple));
      auto const return_type_id = vm->instruction_->type_id;
      auto const result_sp      = vm->sp_ - sp_offset;
      if (result_sp < VM::STACK_SIZE)
      {
        StackSetter<ReturnType>::Set(vm, result_sp, std::move(result), return_type_id);
        vm->sp_ = result_sp;
        return;
      }
      vm->RuntimeError("stack overflow");
    }
  }
};

template <int sp_offset, typename Callable, typename EtchArgsTuple>
struct VmFreeFunctionInvoker<sp_offset, void, Callable, EtchArgsTuple>
{
  template <typename Estimator, typename... ExtraArgs>
  static void Invoke(VM *vm, Estimator &&estimator, Callable &&callable,
                     EtchArgsTuple &&etch_arguments, ExtraArgs const &... extra_args)
  {
    auto estimator_args_tuple = etch_arguments;
    if (EstimateCharge(vm, std::forward<Estimator>(estimator), std::move(estimator_args_tuple)))
    {
      // prepend extra non-etch arguments (e.g. VM*, TypeId)
      auto args_tuple = std::tuple_cat(std::make_tuple(extra_args...),
                                       std::forward<EtchArgsTuple>(etch_arguments));

      meta::Apply(std::forward<Callable>(callable), std::move(args_tuple));
      vm->sp_ -= sp_offset;
    }
  }
};

}  // namespace vm
}  // namespace fetch
