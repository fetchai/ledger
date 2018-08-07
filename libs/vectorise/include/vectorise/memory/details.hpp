#pragma once
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
    typedef typename MatrixApplyFreeFunction<B, R, Args..., B const &>::template Unroll<
        Remaining...>::signature_type signature_type;

    static R Apply(B const *regs, signature_type &&fnc, B &ret, Args &&... args)
    {
      return MatrixApplyFreeFunction<B, R, Args..., B const &>::template Unroll<
          Remaining...>::Apply(regs + 1, std::move(fnc), ret, std::forward<Args>(args)..., *regs);
    }
  };

  template <typename T>
  struct Unroll<T>
  {
    typedef R (*signature_type)(Args..., B const &, B &);
    static R Apply(B const *regs, signature_type &&fnc, B &ret, Args &&... args)
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
    typedef typename MatrixReduceFreeFunction<B, Args..., B const &>::template Unroll<
        Remaining...>::signature_type signature_type;

    static B Apply(B const &self, B const *regs, signature_type &&fnc, Args &&... args)
    {

      return MatrixReduceFreeFunction<B, Args..., B const &>::template Unroll<Remaining...>::Apply(
          self, regs + 1, std::move(fnc), std::forward<Args>(args)..., *regs);
    }
  };

  template <typename T>
  struct Unroll<T>
  {
    //    typedef B(*signature_type)(B const&, Args..., B const&);
    typedef std::function<B(B const &, Args..., B const &)> signature_type;
    static B Apply(B const &self, B const *regs, signature_type &&fnc, Args &&... args)
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
    typedef typename MatrixApplyClassMember<C, B, R, Args..., B const &>::template Unroll<
        Remaining...>::signature_type signature_type;
    static R Apply(B const *regs, C const &cls, signature_type &&fnc, B &ret, Args &&...args)
    {
      return MatrixApplyClassMember<C, B, R, Args..., B const &>::template Unroll<
        Remaining...>::Apply(regs + 1, cls, std::move(fnc), ret, std::forward<Args>(args)..., *regs);
    }

    static R Apply(B const **regs, C const &cls, signature_type &&fnc, B &ret, Args &&...args)
    {
      return MatrixApplyClassMember<C, B, R, Args..., B const &>::template Unroll<
        Remaining...>::Apply(regs + 1, cls, std::move(fnc), ret, std::forward<Args>(args)..., **regs);
    }
  };

  template <typename T>
  struct Unroll<T>
  {
    typedef R (C::*signature_type)(Args..., B const &, B &) const;
    static R Apply(B const *regs, C const &cls, signature_type &&fnc, B &ret, Args &&... args)
    {
      return (cls.*fnc)(std::forward<Args>(args)..., *regs, ret);
    }

    static R Apply(B const **regs, C const &cls, signature_type &&fnc, B &ret, Args &&... args)
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

  static void Apply(A *regs, B *iters) {}
};
}  // namespace details

}  // namespace memory
}  // namespace fetch
