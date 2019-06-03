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

#include <algorithm>
#include <functional>
#include <type_traits>
#include <utility>

namespace fetch {
namespace memory {

namespace details {

template <typename B, typename R, typename... Args>
struct MatrixApplyFreeFunction
{

  template <typename T, typename... Remaining>
  struct Unroll
  {

    template <typename F>
    static R Apply(B const *regs, F &&fnc, B &ret, Args &&... args)
    {
      return MatrixApplyFreeFunction<B, R, Args..., B const &>::template Unroll<
          Remaining...>::Apply(regs + 1, fnc, ret, std::forward<Args>(args)..., *regs);
    }
  };

  template <typename T>
  struct Unroll<T>
  {
    template <typename F>
    static R Apply(B const *regs, F &&fnc, B &ret, Args &&... args)
    {
      return fnc(std::forward<Args>(args)..., *regs, ret);
    }
  };
};

template <typename B, typename... Args>
struct MatrixReduceFreeFunction
{

  template <typename T, typename... Remaining>
  struct Unroll
  {
    using signature_type =
        typename MatrixReduceFreeFunction<B, Args...,
                                          B const &>::template Unroll<Remaining...>::signature_type;

    static B Apply(B const &self, B const *regs, signature_type &&fnc, Args &&... args)
    {

      return MatrixReduceFreeFunction<B, Args..., B const &>::template Unroll<Remaining...>::Apply(
          self, regs + 1, std::move(fnc), std::forward<Args>(args)..., *regs);
    }
  };

  template <typename T>
  struct Unroll<T>
  {
    using signature_type = std::function<B(const B &, Args..., const B &)>;
    static B Apply(B const &self, B const *regs, signature_type const &fnc, Args &&... args)
    {
      return fnc(self, std::forward<Args>(args)..., *regs);
    }
  };
};

template <typename C, typename B, typename R, typename... Args>
struct MatrixApplyClassMember
{

  template <typename T, typename... Remaining>
  struct Unroll
  {
    using signature_type =
        typename MatrixApplyClassMember<C, B, R, Args...,
                                        B const &>::template Unroll<Remaining...>::signature_type;

    static R Apply(B const *regs, C const &cls, signature_type const &fnc, B &ret, Args... args)
    {
      return MatrixApplyClassMember<C, B, R, Args..., B const &>::template Unroll<
          Remaining...>::Apply(regs + 1, cls, fnc, ret, args..., *regs);
    }

    static R Apply(B const **regs, C const &cls, signature_type const &fnc, B &ret, Args... args)
    {
      return MatrixApplyClassMember<C, B, R, Args..., B const &>::template Unroll<
          Remaining...>::Apply(regs + 1, cls, fnc, ret, args..., **regs);
    }
  };

  template <typename T>
  struct Unroll<T>
  {
    using signature_type = R (C::*)(Args..., const B &, B &) const;

    static R Apply(B const *regs, C const &cls, signature_type const &fnc, B &ret, Args... args)
    {
      return (cls.*fnc)(std::forward<Args>(args)..., *regs, ret);
    }

    static R Apply(B const **regs, C const &cls, signature_type const &fnc, B &ret, Args... args)
    {
      return (cls.*fnc)(std::forward<Args>(args)..., **regs, ret);
    }
  };
};

template <std::size_t N, typename A, typename B>
struct UnrollNext
{
  static void Apply(A *regs, B *iters)
  {
    (*iters).Next(*regs);
    UnrollNext<N - 1, A, B>::Apply(regs + 1, iters + 1);
  }
};

template <typename A, typename B>
struct UnrollNext<0, A, B>
{
  static void Apply(A * /*regs*/, B * /*iters*/)
  {}
};
}  // namespace details

}  // namespace memory
}  // namespace fetch
