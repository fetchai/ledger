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
    scalar_size = 8 * sizeof(type),
    vector_size = vectorise::VectorRegisterSize<type>::value
  };
  using VectorRegisterType         = typename vectorise::VectorRegister<type, vector_size>;
  using VectorRegisterIteratorType = vectorise::VectorRegisterIterator<type, vector_size>;
  using ScalarRegisterType         = typename vectorise::VectorRegister<type, scalar_size>;
  using ScalarRegisterIteratorType = vectorise::VectorRegisterIterator<type, scalar_size>;

  template<std::size_t S> 
  using vector_kernel_signature_type = std::function<vectorise::VectorRegister<type, S>(vectorise::VectorRegister<type, S> const, vectorise::VectorRegister<type, S> const)>;
  template<std::size_t S> 
  using vector_reduce_signature_type = std::function<type(vectorise::VectorRegister<type, S> const)>;

  ConstParallelDispatcher(type *ptr, std::size_t size)
    : pointer_(ptr)
    , size_(size)
  {}

  template <std::size_t N, typename G, typename... Args>
  static void InitializeVectorIterators(std::size_t offset, std::size_t size,
                                        vectorise::VectorRegisterIterator<type, N> *iters, G &next,
                                        Args &&... remaining)
  {
    assert(next.padded_size() >= offset + size);
    (*iters) = vectorise::VectorRegisterIterator<type, N>(next.pointer() + offset, size);
    InitializeVectorIterators<N>(offset, size, iters + 1, std::forward<Args>(remaining)...);
  }

  template <std::size_t N, typename G>
  static void InitializeVectorIterators(std::size_t offset, std::size_t size,
                                        vectorise::VectorRegisterIterator<type, N> *iters, G &next)
  {
    assert(next.padded_size() >= offset + size);
    (*iters) = vectorise::VectorRegisterIterator<type, N>(next.pointer() + offset, size);
  }

  template <std::size_t N>
  static void InitializeVectorIterators(std::size_t /*offset*/, std::size_t /*size*/,
                                        vectorise::VectorRegisterIterator<type, N>* /*iters*/)
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

  template <std::size_t S>
  type Reduce(vector_kernel_signature_type<S> &&kernel, vector_reduce_signature_type<S> &&hkernel)
  {
    VectorRegisterType         a, b(type(0));
    VectorRegisterIteratorType iter(this->pointer(), this->size());

    std::size_t N = this->size();

    for (std::size_t i = 0; i < N; i += VectorRegisterType::E_BLOCK_COUNT)
    {
      iter.Next(a);
      b = kernel(a, b);
    }

    return hkernel(b);
  }

  template <std::size_t S, class OP>
  type GenericReduce(Range const &range, OP &&op, type const c, vector_kernel_signature_type<S> &&kernel, vector_reduce_signature_type<S> &&hkernel)
  {
    int SFL      = int(range.SIMDFromLower<VectorRegisterType::E_BLOCK_COUNT>());
    int SF       = int(range.SIMDFromUpper<VectorRegisterType::E_BLOCK_COUNT>());
    int ST       = int(range.SIMDToLower<VectorRegisterType::E_BLOCK_COUNT>());
    int STU      = int(range.SIMDToUpper<VectorRegisterType::E_BLOCK_COUNT>());
    int SIMDSize = STU - SFL;

    type ret = 0;
    std::cout << "Vector iterator:" << std::endl;
    VectorRegisterIteratorType self_iter(this->pointer(), std::size_t(SIMDSize));
    VectorRegisterType         vc(c), tmp, self;

    std::cout << "Scalar iterator:" << std::endl;
    ScalarRegisterIteratorType scalar_self_iter(this->pointer(), std::size_t(SIMDSize));

    ScalarRegisterType b;

    if (SFL != SF)
    {
      std::cout << "SFL: " << SFL << std::endl;
      std::cout << "SF: " << SF << std::endl;

      std::cout << "Scalar iterator:" << std::endl;
      ScalarRegisterIteratorType scalar_iter(self_iter, std::size_t(SF)/sizeof(type));
      ScalarRegisterType scalar_self, scalar_tmp;

      self_iter.Next(self);

      while (static_cast<void*>(scalar_iter.pointer()) < static_cast<void*>(self_iter.pointer()))
      {
        scalar_iter.Next(scalar_self);
        std::cout << "self = " << scalar_self << std::endl;
        std::cout << "b = " << b << std::endl;
        scalar_tmp = kernel(scalar_self);
        std::cout << "scalar_tmp = " << scalar_tmp << std::endl;
        b = op(b, scalar_tmp);
        std::cout << "b = " << b << std::endl;
      }
      vc = VectorRegisterType(b.data());
    }

    for (int i = SF; i < ST; i += VectorRegisterType::E_BLOCK_COUNT)
    {
      self_iter.Next(self);
      tmp = kernel(self);
      vc  = op(vc, tmp);
    }

    ret += hkernel(vc);

    // Taking care of the tail
    if (STU != ST)
    {
      self_iter.Next(self);
      tmp = kernel(self);

      int Q = (int(range.to()) - ST - 1);
      for (int i = 0; i <= Q; ++i)
      {
        ret += first_element(tmp);
        tmp = shift_elements_right(tmp);
      }
    }

    return ret;
  }

  template <std::size_t S>
  type SumReduce(Range const &range, vector_kernel_signature_type<S> &&kernel, vector_reduce_signature_type<S> &&hkernel)
  {
    return GenericReduce(range, std::plus<VectorRegisterType>{}, type(0), kernel, hkernel);
  }

  template <std::size_t S>
  type ProductReduce(Range const &range, vector_kernel_signature_type<S> &&kernel, vector_reduce_signature_type<S> &&hkernel)
  {
    return GenericReduce(range, std::multiplies<VectorRegisterType>{}, type(1), kernel, hkernel);
  }

  template <std::size_t S, class OP, typename... Args>
  type GenericReduce(Range const &range, OP const &&op, type const c, vector_kernel_signature_type<S> &&kernel, vector_reduce_signature_type<S> &&hkernel, Args &&... args)
  {
    int SFL      = int(range.SIMDFromLower<VectorRegisterType::E_BLOCK_COUNT>());
    int SF       = int(range.SIMDFromUpper<VectorRegisterType::E_BLOCK_COUNT>());
    int ST       = int(range.SIMDToLower<VectorRegisterType::E_BLOCK_COUNT>());
    int STU      = int(range.SIMDToUpper<VectorRegisterType::E_BLOCK_COUNT>());
    int SIMDSize = STU - SFL;

    VectorRegisterType         regs[sizeof...(args)];
    VectorRegisterIteratorType iters[sizeof...(args)];
    std::cout << "Vector iterator:" << std::endl;
    VectorRegisterIteratorType self_iter(this->pointer(), std::size_t(SIMDSize));
    VectorRegisterType         vc(c), tmp, self;
    InitializeVectorIterators<vector_size>(0, std::size_t(SIMDSize), iters, std::forward<Args>(args)...);

    // Taking care of head
    type ret{0};
    if (SFL != SF)
    {
      std::cout << "SFL: " << SFL << std::endl;
      std::cout << "SF: " << SF << std::endl;

      ScalarRegisterType         scalar_regs[sizeof...(args)];
      ScalarRegisterIteratorType scalar_iters[sizeof...(args)];
      std::cout << "Scalar iterator:" << std::endl;
      ScalarRegisterIteratorType scalar_self_iter(self_iter, std::size_t(SF)/sizeof(type));
      ScalarRegisterType scalar_self, scalar_tmp;
      InitializeVectorIterators<scalar_size>(0, std::size_t(SIMDSize), scalar_iters, std::forward<Args>(args)...);

      details::UnrollNext<sizeof...(args), VectorRegisterType, VectorRegisterIteratorType>::Apply(
          regs, iters);
      self_iter.Next(self);
      tmp = details::MatrixReduceFreeFunction<VectorRegisterType>::template Unroll<Args...>::Apply(
          self, regs, kernel);

      while (static_cast<void*>(scalar_self_iter.pointer()) < static_cast<void*>(self_iter.pointer()))
      {
        details::UnrollNext<sizeof...(args), ScalarRegisterType, ScalarRegisterIteratorType>::Apply(
          scalar_regs, scalar_iters);
        scalar_self_iter.Next(scalar_self);
        // scalar_tmp = details::MatrixReduceFreeFunction<ScalarRegisterType>::template Unroll<Args...>::Apply(
        //   scalar_self, scalar_regs, kernel);
        std::cout << "self = " << scalar_self << std::endl;
        std::cout << "tmp = " << scalar_tmp << std::endl;
        std::cout << "c = " << ret << std::endl;
        ret += scalar_tmp.data();
        std::cout << "c = " << ret << std::endl;
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

    ret += hkernel(vc);

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

  template <std::size_t S, typename... Args>
  type SumReduce(Range const &range, vector_kernel_signature_type<S> &&kernel, vector_reduce_signature_type<S> &&hkernel, Args &&... args)
  {
    return GenericReduce(range, std::plus<VectorRegisterType>{}, type(0), kernel, hkernel, std::forward<Args>(args)...);
  }

  template <std::size_t S>
  type Reduce(Range const &range, vector_kernel_signature_type<S> &&kernel, vector_reduce_signature_type<S> &&hkernel)
  {
    int SFL = int(range.SIMDFromLower<VectorRegisterType::E_BLOCK_COUNT>());
    int SF = int(range.SIMDFromUpper<VectorRegisterType::E_BLOCK_COUNT>());
    int ST = int(range.SIMDToLower<VectorRegisterType::E_BLOCK_COUNT>());
    int STU      = int(range.SIMDToUpper<VectorRegisterType::E_BLOCK_COUNT>());
    int SIMDSize = STU - SFL;

    VectorRegisterType         va, vb(type(0));
    VectorRegisterIteratorType iter(this->pointer() + SFL, std::size_t(SIMDSize));
    ScalarRegisterType b;

    if (SFL != SF)
    {
      std::cout << "SFL: " << SFL << std::endl;
      std::cout << "SF: " << SF << std::endl;

      std::cout << "Scalar iterator:" << std::endl;
      ScalarRegisterIteratorType scalar_iter(iter, std::size_t(SF)/sizeof(type));
      ScalarRegisterType a;

      iter.Next(va);

      while (static_cast<void*>(scalar_iter.pointer()) < static_cast<void*>(iter.pointer()))
      {
        scalar_iter.Next(a);
        std::cout << "self = " << a << std::endl;
        std::cout << "b = " << b << std::endl;
        b = kernel(a, b);
        std::cout << "b = " << b << std::endl;
      }
      vb = VectorRegisterType(b.data());
    }

    std::cout << "vb = " << vb << std::endl;
    std::cout << "Vector iterator:" << std::endl;
    for (int i = SF; i < ST; i += VectorRegisterType::E_BLOCK_COUNT)
    {
      iter.Next(va);
      std::cout << "va = " << va << std::endl;
      std::cout << "vb = " << vb << std::endl;
      vb = kernel(va, vb);
      std::cout << "vb = " << vb << std::endl;
    }

    std::cout << "vb = " << vb << std::endl;
    std::cout << "b = " << b << std::endl;
    b.data() = hkernel(vb);
    std::cout << "b = " << b << std::endl;

    if (STU != ST)
    {
      std::cout << "STU: " << STU << std::endl;
      std::cout << "ST: " << ST << std::endl;

      std::cout << "Scalar iterator:" << std::endl;
      ScalarRegisterIteratorType scalar_iter(iter, std::size_t(SF)/sizeof(type));
      ScalarRegisterType a, b;

      while (static_cast<void*>(scalar_iter.pointer()) < static_cast<void*>(iter.end()))
      {
        scalar_iter.Next(a);
        std::cout << "self = " << a << std::endl;
        std::cout << "b = " << b << std::endl;
        b = kernel(a, b);
        std::cout << "b = " << b << std::endl;
      }
    }

    return b.data();
  }

  template <std::size_t S, class OP, typename... Args>
  type GenericReduce(OP &&op, type const c, vector_kernel_signature_type<S> &&kernel, vector_reduce_signature_type<S> &&hkernel, Args &&... args)
  {
    VectorRegisterType         regs[sizeof...(args)];
    VectorRegisterIteratorType iters[sizeof...(args)];
    InitializeVectorIterators<vector_size>(0, this->size(), iters, std::forward<Args>(args)...);

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

    return hkernel(vc);
  }

  template <std::size_t S, typename... Args>
  type SumReduce(vector_kernel_signature_type<S> &&kernel, vector_reduce_signature_type<S> &&hkernel, Args &&... args)
  {
    return GenericReduce(std::plus<VectorRegisterType>{}, type(0), kernel, hkernel, std::forward<Args>(args)...);
  }

  template <std::size_t S, typename... Args>
  type ProductReduce(vector_kernel_signature_type<S> &&kernel, vector_reduce_signature_type<S> &&hkernel, Args &&... args)
  {
    return GenericReduce(std::multiplies<VectorRegisterType>{}, type(1), kernel, hkernel, std::forward<Args>(args)...);
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
      std::cout << "STU = " << STU << std::endl;
      std::cout << "ST = " << ST << std::endl;
      std::cout << "range.to() = " << range.to() << std::endl;
      details::UnrollNext<sizeof...(args), VectorRegisterType, VectorRegisterIteratorType>::Apply(
          regs, iters);
      details::MatrixApplyFreeFunction<VectorRegisterType, void>::template Unroll<Args...>::Apply(
          regs, apply, c);

      int Q = (int(range.to()) - ST - 1);
      std::cout << "Q = " << Q << std::endl;
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
