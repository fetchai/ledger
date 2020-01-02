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

#include "core/macros.hpp"
#include "meta/callable/callable_traits.hpp"
#include "vm/module/base.hpp"
#include "vm/vm.hpp"

#include <type_traits>

namespace fetch {
namespace vm {

namespace internal {

template <typename EtchArgumentTuple, template <int, typename, typename, typename...> class Invoker,
          typename ReturnType>
struct EtchArgumentTupleComposer
{
  static constexpr auto Compose(VM *vm)
  {
    return ComposeUnpacked(vm, meta::IndexSequenceFromTuple<EtchArgumentTuple>());
  }

private:
  static constexpr auto FIRST_PARAMETER_OFFSET = std::tuple_size<EtchArgumentTuple>::value - 1;

  template <std::size_t... Is>
  static auto ComposeUnpacked(VM *vm, meta::IndexSequence<Is...> && /*unused*/)
  {
    FETCH_UNUSED(vm);
    return std::make_tuple(StackGetter<std::tuple_element_t<Is, EtchArgumentTuple>>::Get(
        vm, FIRST_PARAMETER_OFFSET - Is)...);
  }
};

template <template <int, typename, typename, typename> class Invoker, typename Callable,
          typename EtchArgsTuple>
class PrepareInvocationImpl;
template <template <int, typename, typename, typename> class Invoker, typename Callable,
          typename... EtchArgs>
class PrepareInvocationImpl<Invoker, Callable, std::tuple<EtchArgs...>>
{
  constexpr static int num_parameters = int(sizeof...(EtchArgs));
  constexpr static int sp_offset =
      meta::CallableTraits<Callable>::is_void() ? num_parameters : (num_parameters - 1);

  using ReturnType = typename meta::CallableTraits<Callable>::ReturnType;

public:
  using ConfiguredInvoker = Invoker<sp_offset, ReturnType, Callable, std::tuple<EtchArgs...>>;

  static auto GetEtchArguments(VM *vm)
  {
    return EtchArgumentTupleComposer<std::tuple<EtchArgs...>, Invoker, ReturnType>::Compose(vm);
  }
};

}  // namespace internal

template <template <int, typename, typename, typename> class Invoker, typename Callable,
          typename EtchArgsTuple>
using PrepareInvocation =
    typename internal::PrepareInvocationImpl<Invoker, Callable, EtchArgsTuple>;

}  // namespace vm
}  // namespace fetch
