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

#include "meta/pack.hpp"
#include "meta/type_traits.hpp"
#include "meta/type_util.hpp"

#include <functional>

namespace fetch {
namespace value_util {

namespace detail_ {

template <class Arg, class F, class Args = pack::Nil, bool = pack::IsInvocableV<F, Args>>
struct Arity : pack::TupleSize<Args>
{
};

template <class Arg, class F>
static constexpr auto ArityV = Arity<Arg, F>::value;

template <class Arg, class F, class Args>
struct Arity<Arg, F, Args, false> : Arity<Arg, F, pack::ConsT<Arg, Args>>
{
  static_assert(pack::TupleSizeV<Args> < 256,
                "Function's arity is too large (or function is not invocable at all with such "
                "argument type)");
};

template <class RV, class... Args>
using Fn = std::function<RV(Args...)>;

template <class RV, class Arg, class Aty>
using SlotImpl =
    pack::ApplyT<pack::Bind<Fn, pack::Singleton<RV>>::template type, pack::RepeatT<Aty, Arg>>;

template <class Arg, class F>
struct Slot
{
  using Aty  = Arity<Arg, F>;
  using Args = pack::RepeatT<Aty, Arg>;
  using RV   = pack::InvokeResultT<F, Args>;

  using type = SlotImpl<RV, Arg, Aty>;
};

template <class Arg, class F>
using SlotT = typename Slot<Arg, F>::type;

}  // namespace detail_

template <class F, class Arg>
struct SlotType : detail_::SlotT<Arg, F>
{
  using detail_::SlotT<Arg, F>::SlotT;
};

template <class F, class... Args>
struct SlotType<F, pack::Pack<Args...>> : SlotType<F, Args>...
{
  SlotType(F const &f)
    : SlotType<F, Args>(f)...
  {}
};

template <class... Args, class F>
constexpr auto Slot(F &&f)
{
  return SlotType<F, pack::ConcatT<Args...>>(std::forward<F>(f));
}

template <class... Fs>
struct SlotsType : Fs...
{
  constexpr SlotsType(Fs &&... fs)
    : Fs(std::forward<Fs>(fs))...
  {}
};

template <class... Fs>
constexpr auto Slots(Fs &&... fs)
{
  return SlotsType<Fs...>(std::forward<Fs>(fs)...);
}

}  // namespace value_util
}  // namespace fetch
