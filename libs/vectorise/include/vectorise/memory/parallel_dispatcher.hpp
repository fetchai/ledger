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

#include "vectorise/memory/details.hpp"
#include "vectorise/memory/range.hpp"
#include "vectorise/platform.hpp"
#include "vectorise/vectorise.hpp"

#include <iostream>
#include <type_traits>

namespace fetch {
namespace memory {

template <typename T>
class ConstParallelDispatcher
{
public:
  using type = T;

  enum
  {
    vector_size = vectorise::VectorRegisterSize<type>::value
  };
  using VectorRegisterType         = typename vectorise::VectorRegister<type, vector_size>;
  using VectorRegisterIteratorType = vectorise::VectorRegisterIterator<type, vector_size>;

  ConstParallelDispatcher(type *ptr, std::size_t size)
    : pointer_(ptr)
    , size_(size)
  {}

  type Reduce(VectorRegisterType (*vector_reduction)(VectorRegisterType const &,
                                                     VectorRegisterType const &)) const
  {
    VectorRegisterType         a, b(type(0));
    VectorRegisterIteratorType iter(this->pointer(), this->size());

    std::size_t N = this->size();

    for (std::size_t i = 0; i < N; i += VectorRegisterType::E_BLOCK_COUNT)
    {
      iter.Next(a);
      b = vector_reduction(a, b);
    }

    return reduce(b);
  }

  template <typename F>
  type SumReduce(Range const &range, F &&vector_reduce)
  {
    int SFL      = int(range.SIMDFromLower<VectorRegisterType::E_BLOCK_COUNT>());
    int SF       = int(range.SIMDFromUpper<VectorRegisterType::E_BLOCK_COUNT>());
    int ST       = int(range.SIMDToLower<VectorRegisterType::E_BLOCK_COUNT>());
    int STU      = int(range.SIMDToUpper<VectorRegisterType::E_BLOCK_COUNT>());
    int SIMDSize = STU - SFL;

    type                       ret = 0;
    VectorRegisterIteratorType self_iter(this->pointer(), std::size_t(SIMDSize));
    VectorRegisterType         c(type(0)), tmp, self;

    // Taking care of head
    if (SFL != SF)
    {
      self_iter.Next(self);
      tmp = vector_reduce(self);

      int Q = VectorRegisterType::E_BLOCK_COUNT - (SF - int(range.from()));
      for (int i = 0; i < VectorRegisterType::E_BLOCK_COUNT; ++i)
      {
        if (Q <= i)
        {
          ret += first_element(tmp);
        }
        tmp = shift_elements_right(tmp);
      }
    }

    for (int i = SF; i < ST; i += VectorRegisterType::E_BLOCK_COUNT)
    {
      self_iter.Next(self);
      tmp = vector_reduce(self);
      c   = c + tmp;
    }

    ret += reduce(c);

    // Taking care of the tail
    if (STU != ST)
    {
      self_iter.Next(self);
      tmp = vector_reduce(self);

      int Q = (int(range.to()) - ST - 1);
      for (int i = 0; i <= Q; ++i)
      {
        ret += first_element(tmp);
        tmp = shift_elements_right(tmp);
      }
    }

    return ret;
  }
  // @}

  template <typename... Args>
  type SumReduce(typename details::MatrixReduceFreeFunction<VectorRegisterType>::
                         template Unroll<Args...>::signature_type const &kernel,
                     Args &&... args)
  {
    return GenericReduce(std::plus<VectorRegisterType>{}, kernel, type(0), std::forward<Args>(args)...);
  }

  template <typename... Args>
  type ProductReduce(typename details::MatrixReduceFreeFunction<VectorRegisterType>::
                         template Unroll<Args...>::signature_type const &kernel,
                     Args &&... args)
  {
    return GenericReduce(std::multiplies<VectorRegisterType>{}, kernel, type(1), std::forward<Args>(args)...);
  }

  template <class F, typename... Args>
  type GenericReduce(F &&op, typename details::MatrixReduceFreeFunction<VectorRegisterType>::
                         template Unroll<Args...>::signature_type const &kernel, type c,
                     Args &&... args)
  {
    VectorRegisterType         regs[sizeof...(args)];
    VectorRegisterIteratorType iters[sizeof...(args)];
    InitializeVectorIterators(0, this->size(), iters, std::forward<Args>(args)...);

    VectorRegisterIteratorType self_iter(this->pointer(), this->size());
    VectorRegisterType         vc(c), tmp, self;

    std::size_t N = this->size();
    for (std::size_t i = 0; i < N; i += VectorRegisterType::E_BLOCK_COUNT)
    {
      details::UnrollNext<sizeof...(args), VectorRegisterType, VectorRegisterIteratorType>::Apply(
          regs, iters);
      self_iter.Next(self);
      tmp = details::MatrixReduceFreeFunction<VectorRegisterType>::template Unroll<Args...>::Apply(
          self, regs, kernel);
      std::cout << "self = " << self << std::endl;
      std::cout << "tmp = " << tmp << std::endl;
      std::cout << "c = " << c << std::endl;
      vc = op(vc, tmp);
      std::cout << "c = " << c << std::endl;
    }

    return reduce(vc);
  }

  template <typename... Args>
  type SumReduce(Range const &range, typename details::MatrixReduceFreeFunction<VectorRegisterType>::
                         template Unroll<Args...>::signature_type const &kernel,
                     Args &&... args)
  {
    return GenericReduce(range, std::plus<VectorRegisterType>{}, kernel, type(0), std::forward<Args>(args)...);
  }

  template <class F, typename... Args>
  type GenericReduce(Range const &range, F &&op, typename details::MatrixReduceFreeFunction<VectorRegisterType>::
                         template Unroll<Args...>::signature_type const &kernel, type c,
                     Args &&... args)
  {
    int SFL      = int(range.SIMDFromLower<VectorRegisterType::E_BLOCK_COUNT>());
    int SF       = int(range.SIMDFromUpper<VectorRegisterType::E_BLOCK_COUNT>());
    int ST       = int(range.SIMDToLower<VectorRegisterType::E_BLOCK_COUNT>());
    int STU      = int(range.SIMDToUpper<VectorRegisterType::E_BLOCK_COUNT>());
    int SIMDSize = STU - SFL;

    VectorRegisterType         regs[sizeof...(args)];
    VectorRegisterIteratorType iters[sizeof...(args)];
    InitializeVectorIterators(0, std::size_t(SIMDSize), iters, std::forward<Args>(args)...);
    VectorRegisterIteratorType self_iter(this->pointer(), std::size_t(SIMDSize));
    VectorRegisterType         vc(c), tmp, self;

    // Taking care of head
    type ret{0};
    if (SFL != SF)
    {
      details::UnrollNext<sizeof...(args), VectorRegisterType, VectorRegisterIteratorType>::Apply(
          regs, iters);
      self_iter.Next(self);
      tmp = details::MatrixReduceFreeFunction<VectorRegisterType>::template Unroll<Args...>::Apply(
          self, regs, kernel);

      int Q = VectorRegisterType::E_BLOCK_COUNT - (SF - int(range.from()));
      for (int i = 0; i < VectorRegisterType::E_BLOCK_COUNT; ++i)
      {
        if (Q <= i)
        {
          ret += first_element(tmp);
        }
        tmp = shift_elements_right(tmp);
      }
    }

    for (int i = SF; i < ST; i += VectorRegisterType::E_BLOCK_COUNT)
    {
      details::UnrollNext<sizeof...(args), VectorRegisterType, VectorRegisterIteratorType>::Apply(
          regs, iters);
      self_iter.Next(self);
      tmp = details::MatrixReduceFreeFunction<VectorRegisterType>::template Unroll<Args...>::Apply(
          self, regs, kernel);
      std::cout << "self = " << self << std::endl;
      std::cout << "tmp = " << tmp << std::endl;
      std::cout << "c = " << vc << std::endl;
      vc = op(vc, tmp);
      std::cout << "c = " << vc << std::endl;
    }

    ret += reduce(vc);

    // Taking care of the tail
    if (STU != ST)
    {
      details::UnrollNext<sizeof...(args), VectorRegisterType, VectorRegisterIteratorType>::Apply(
          regs, iters);
      self_iter.Next(self);
      tmp = details::MatrixReduceFreeFunction<VectorRegisterType>::template Unroll<Args...>::Apply(
          self, regs, kernel);

      int Q = (int(range.to()) - ST - 1);
      for (int i = 0; i <= Q; ++i)
      {
        ret += first_element(tmp);
        tmp = shift_elements_right(tmp);
      }
    }

    return ret;
  }

  type Reduce(Range const &range,
              VectorRegisterType (*vector_reduction)(VectorRegisterType const &,
                                                     VectorRegisterType const &)) const
  {
    int SFL = int(range.SIMDFromLower<VectorRegisterType::E_BLOCK_COUNT>());

    int SF = int(range.SIMDFromUpper<VectorRegisterType::E_BLOCK_COUNT>());
    int ST = int(range.SIMDToLower<VectorRegisterType::E_BLOCK_COUNT>());

    int STU      = int(range.SIMDToUpper<VectorRegisterType::E_BLOCK_COUNT>());
    int SIMDSize = STU - SFL;

    VectorRegisterType         a, b(type(0));
    VectorRegisterIteratorType iter(this->pointer() + SFL, std::size_t(SIMDSize));

    if (SFL != SF)
    {
      iter.Next(a);
      a = vector_zero_below_element(a,
                                    VectorRegisterType::E_BLOCK_COUNT - (SF - int(range.from())));
      b = vector_reduction(a, b);
    }

    for (int i = SF; i < ST; i += VectorRegisterType::E_BLOCK_COUNT)
    {
      iter.Next(a);
      b = vector_reduction(a, b);
    }

    if (STU != ST)
    {
      iter.Next(a);
      a = vector_zero_above_element(a, (int(range.to()) - ST - 1));
      b = vector_reduction(a, b);
    }

    // TODO(issue 1): Make reduction tree / Wallace tree
    type ret = 0;
    for (std::size_t i = 0; i < VectorRegisterType::E_BLOCK_COUNT; ++i)
    {
      VectorRegisterType c(ret);
      c   = vector_reduction(c, b);
      ret = first_element(c);
      b   = shift_elements_right(b);
    }

    return ret;
  }

  type Reduce(type (*register_reduction)(type const &, type const &)) const
  {
    type        ret = 0;
    std::size_t N   = this->size();

    for (std::size_t i = 0; i < N; ++i)
    {
      ret = register_reduction(ret, pointer_[i]);
    }

    return ret;
  }

  type Reduce(Range const &range,
              type (*register_reduction)(type const &, type const &)) const
  {
    type ret = 0;
    for (std::size_t i = range.from(); i < range.to(); ++i)
    {
      ret = register_reduction(ret, pointer_[i]);
    }
    return ret;
  }

  type const *pointer() const
  {
    return pointer_;
  }

  std::size_t size() const
  {
    return size_;
  }

  std::size_t &size()
  {
    return size_;
  }

protected:
  type *pointer()
  {
    return pointer_;
  }

  type *      pointer_;
  std::size_t size_;

  template <typename G, typename... Args>
  static void InitializeVectorIterators(std::size_t offset, std::size_t size,
                                        VectorRegisterIteratorType *iters, G &next,
                                        Args &&... remaining)
  {
    assert(next.padded_size() >= offset + size);
    (*iters) = VectorRegisterIteratorType(next.pointer() + offset, size);
    InitializeVectorIterators(offset, size, iters + 1, std::forward<Args>(remaining)...);
  }

  template <typename G>
  static void InitializeVectorIterators(std::size_t offset, std::size_t size,
                                        VectorRegisterIteratorType *iters, G &next)
  {
    assert(next.padded_size() >= offset + size);
    (*iters) = VectorRegisterIteratorType(next.pointer() + offset, size);
  }

  static void InitializeVectorIterators(std::size_t /*offset*/, std::size_t /*size*/,
                                        VectorRegisterIteratorType* /*iters*/)
  {}

  template <typename G, typename... Args>
  static void SetPointers(std::size_t offset, std::size_t size, type const **regs, G &next,
                          Args &&... remaining)
  {
    assert(next.size() >= offset + size);
    *regs = next.pointer() + offset;
    SetPointers(offset, size, regs + 1, std::forward<Args>(remaining)...);
  }

  template <typename G>
  static void SetPointers(std::size_t offset, std::size_t size, type const **regs, G &next)
  {
    assert(next.size() >= offset + size);
    (void)size;
    *regs = next.pointer() + offset;
  }

  static void SetPointers(std::size_t /*offset*/, std::size_t /*size*/,
                          VectorRegisterIteratorType * /*iters*/)
  {}
};

template <typename T>
class ParallelDispatcher : public ConstParallelDispatcher<T>
{
public:
  using type = T;

  using super_type = ConstParallelDispatcher<T>;

  enum
  {
    vector_size = vectorise::VectorRegisterSize<type>::value
  };
  using VectorRegisterType         = typename vectorise::VectorRegister<type, vector_size>;
  using VectorRegisterIteratorType = vectorise::VectorRegisterIterator<type, vector_size>;

  ParallelDispatcher(type *ptr, std::size_t size)
    : super_type(ptr, size)
  {}

  template <typename F>
  void Apply(F &&apply)
  {
    VectorRegisterType c;

    std::size_t N = this->size();
    for (std::size_t i = 0; i < N; i += VectorRegisterType::E_BLOCK_COUNT)
    {
      assert(i < this->size());
      apply(c);

      c.Store(this->pointer() + i);
    }
  }

  // TODO (issue 1113): This function is potentially slow
  template <typename... Args>
  void Apply(typename details::MatrixApplyFreeFunction<VectorRegisterType, void>::template Unroll<
                 Args...>::signature_type &&apply,
             Args &&... args)
  {
    VectorRegisterType         regs[sizeof...(args)], c;
    VectorRegisterIteratorType iters[sizeof...(args)];
    ConstParallelDispatcher<T>::InitializeVectorIterators(0, this->size(), iters,
                                                          std::forward<Args>(args)...);

    std::size_t N = this->size();
    for (std::size_t i = 0; i < N; i += VectorRegisterType::E_BLOCK_COUNT)
    {
      details::UnrollNext<sizeof...(args), VectorRegisterType, VectorRegisterIteratorType>::Apply(
          regs, iters);

      details::MatrixApplyFreeFunction<VectorRegisterType, void>::template Unroll<Args...>::Apply(
          regs, std::move(apply), c);

      c.Store(this->pointer() + i);
    }
  }

  template <typename F>
  void Apply(Range const &range, F &&apply)
  {
    int SFL = int(range.SIMDFromLower<VectorRegisterType::E_BLOCK_COUNT>());

    int SF = int(range.SIMDFromUpper<VectorRegisterType::E_BLOCK_COUNT>());
    int ST = int(range.SIMDToLower<VectorRegisterType::E_BLOCK_COUNT>());

    int STU = int(range.SIMDToUpper<VectorRegisterType::E_BLOCK_COUNT>());

    VectorRegisterType c;

    if (SFL != SF)
    {
      apply(c);

      int Q = VectorRegisterType::E_BLOCK_COUNT - (SF - int(range.from()));
      for (int i = 0; i < VectorRegisterType::E_BLOCK_COUNT; ++i)
      {
        type value = first_element(c);
        c          = shift_elements_right(c);
        if (Q <= i)
        {
          this->pointer()[SFL + i] = value;
        }
      }
    }

    for (int i = SF; i < ST; i += VectorRegisterType::E_BLOCK_COUNT)
    {
      apply(c);
      c.Store(this->pointer() + i);
    }

    if (STU != ST)
    {
      apply(c);

      int Q = (int(range.to()) - ST - 1);
      for (int i = 0; i <= Q; ++i)
      {
        type value              = first_element(c);
        c                       = shift_elements_right(c);
        this->pointer()[ST + i] = value;
      }
    }
  }

  template <typename F, typename... Args>
  void Apply(Range const &range, F &&apply, Args &&... args)
  {
    int SFL = int(range.SIMDFromLower<VectorRegisterType::E_BLOCK_COUNT>());

    int SF = int(range.SIMDFromUpper<VectorRegisterType::E_BLOCK_COUNT>());
    int ST = int(range.SIMDToLower<VectorRegisterType::E_BLOCK_COUNT>());

    int STU      = int(range.SIMDToUpper<VectorRegisterType::E_BLOCK_COUNT>());
    int SIMDSize = STU - SFL;

    VectorRegisterType         regs[sizeof...(args)], c;
    VectorRegisterIteratorType iters[sizeof...(args)];

    ConstParallelDispatcher<T>::InitializeVectorIterators(std::size_t(SFL), std::size_t(SIMDSize),
                                                          iters, std::forward<Args>(args)...);

    if (SFL != SF)
    {
      details::UnrollNext<sizeof...(args), VectorRegisterType, VectorRegisterIteratorType>::Apply(
          regs, iters);
      details::MatrixApplyFreeFunction<VectorRegisterType, void>::template Unroll<Args...>::Apply(
          regs, apply, c);

      int Q = VectorRegisterType::E_BLOCK_COUNT - (SF - int(range.from()));
      for (int i = 0; i < VectorRegisterType::E_BLOCK_COUNT; ++i)
      {
        type value = first_element(c);
        c          = shift_elements_right(c);
        if (Q <= i)
        {
          this->pointer()[SFL + i] = value;
        }
      }
    }

    for (int i = SF; i < ST; i += VectorRegisterType::E_BLOCK_COUNT)
    {
      details::UnrollNext<sizeof...(args), VectorRegisterType, VectorRegisterIteratorType>::Apply(
          regs, iters);
      details::MatrixApplyFreeFunction<VectorRegisterType, void>::template Unroll<Args...>::Apply(
          regs, apply, c);

      c.Store(this->pointer() + i);
    }

    if (STU != ST)
    {
      details::UnrollNext<sizeof...(args), VectorRegisterType, VectorRegisterIteratorType>::Apply(
          regs, iters);
      details::MatrixApplyFreeFunction<VectorRegisterType, void>::template Unroll<Args...>::Apply(
          regs, apply, c);

      int Q = (int(range.to()) - ST - 1);
      for (int i = 0; i <= Q; ++i)
      {
        type value              = first_element(c);
        c                       = shift_elements_right(c);
        this->pointer()[ST + i] = value;
      }
    }
  }

  type *pointer()
  {
    return super_type::pointer();
  }
};

}  // namespace memory
}  // namespace fetch
