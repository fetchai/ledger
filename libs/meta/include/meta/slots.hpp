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

template <class... As, class F>
struct Slot<pack::Pack<As...>, F>
{
  using Args = pack::Pack<As...>;
  using Aty  = pack::TupleSize<Args>;
  using RV   = pack::InvokeResultT<F, Args>;

  using type = Fn<RV, As...>;
};

// "pack expansion in using-declaration only available with ‘-std=c++17’ or ‘-std=gnu++17’"
// So we'll need a few preparations to have it in C++14.

template <class FirstParent, class OtherParents>
struct FunctorChild : FirstParent, OtherParents
{
  template <class ConstructionArg>
  constexpr FunctorChild(ConstructionArg const &construction_arg)
    : FirstParent(construction_arg)
    , OtherParents(construction_arg)
  {}

  template <class FirstArg, class... OtherArgs>
  constexpr FunctorChild(FirstArg &&first_arg, OtherArgs &&... other_args)
    : FirstParent(std::forward<FirstArg>(first_arg))
    , OtherParents(std::forward<OtherArgs>(other_args)...)
  {}

  using FirstParent:: operator();
  using OtherParents::operator();
};

template <class Arg, class F>
using SlotT = typename Slot<Arg, F>::type;

}  // namespace detail_

template <class F, class... ArgSets>
class SlotType : type_util::ReverseAccumulateT<detail_::FunctorChild, SlotType<F, ArgSets>...>
{
  using Parent = type_util::ReverseAccumulateT<detail_::FunctorChild, SlotType<F, ArgSets>...>;

public:
  SlotType(F const &f)
    : Parent(f)
  {}

  using Parent::operator();
};

template <class F, class Args>
class SlotType<F, Args> : detail_::SlotT<Args, F>
{
  using Parent = detail_::SlotT<Args, F>;

public:
  using Parent::Parent;
  using Parent::operator();
};

template <class... ArgSets, class F>
constexpr auto Slot(F &&f)
{
  return SlotType<F, ArgSets...>(std::forward<F>(f));
}

template <class... Fs>
class SlotsType : type_util::ReverseAccumulateT<detail_::FunctorChild, Fs...>
{
  using Parent = type_util::ReverseAccumulateT<detail_::FunctorChild, Fs...>;

public:
  constexpr SlotsType(Fs &&... fs)
    : Parent(std::forward<Fs>(fs)...)
  {}

  using Parent::operator();
};

template <class... Fs>
constexpr auto Slots(Fs &&... fs)
{
  return SlotsType<Fs...>(std::forward<Fs>(fs)...);
}

}  // namespace value_util
}  // namespace fetch
