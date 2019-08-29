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

  template <class OP, class F1, class F2>
  inline type GenericRangedOpReduce(Range const &range, type const initial_value, OP &&op, F1 const &&kernel, F2 &&hkernel)
  {
    int SFL      = int(range.SIMDFromLower<VectorRegisterType::E_BLOCK_COUNT>());
    int SF       = int(range.SIMDFromUpper<VectorRegisterType::E_BLOCK_COUNT>());
    int ST       = int(range.SIMDToLower<VectorRegisterType::E_BLOCK_COUNT>());
    int STU      = int(range.SIMDToUpper<VectorRegisterType::E_BLOCK_COUNT>());

    type ret{initial_value};
    VectorRegisterType         va, vc(initial_value);
    VectorRegisterIteratorType iter(this->pointer() + SFL, range.to());
    ScalarRegisterType b(initial_value);

    if (SFL != SF)
    {
      ScalarRegisterIteratorType scalar_iter(iter, std::size_t(SF)/sizeof(type));
      ScalarRegisterType a, tmp;

      iter.Next(va);

      while (static_cast<void*>(scalar_iter.pointer()) < static_cast<void*>(iter.pointer()))
      {
        scalar_iter.Next(a);
        tmp = kernel(a);
        ret = op(ret, tmp.data());
      }
    }

    for (int i = SF; i < ST; i += VectorRegisterType::E_BLOCK_COUNT)
    {
      VectorRegisterType tmp;
      iter.Next(va);
      tmp = kernel(va);
      vc  = op(vc, tmp);
    }

    ret += hkernel(vc);

    if (STU != ST)
    {
      ScalarRegisterIteratorType scalar_iter(iter, 0);
      ScalarRegisterType a, tmp;

      while (static_cast<void*>(scalar_iter.pointer()) < static_cast<void*>(iter.end()))
      {
        scalar_iter.Next(a);
        tmp = kernel(a);
        ret = op(ret, tmp.data());
      }
    }

    return ret;
  }

  template <class F1, class F2>
  type SumReduce(F1 const &&kernel, F2 &&hkernel)
  {
    Range range(0, this->size());
    return GenericRangedOpReduce(range, type(0), std::plus<void>{}, std::move(kernel), hkernel);
  }

  template <class F1, class F2>
  type SumReduce(Range const &range, F1 const &&kernel, F2 &&hkernel)
  {
    return GenericRangedOpReduce(range, type(0), std::plus<void>{}, std::move(kernel), hkernel);
  }

  template <class F1, class F2>
  type ProductReduce(Range const &range, F1 const &&kernel, F2 &&hkernel)
  {
    return GenericRangedOpReduce(range, type(1), std::multiplies<void>{}, std::move(kernel), hkernel);
  }

  template <class F1, class F2, class OP,typename... Args>
  inline type GenericRangedReduceMultiple(Range const &range, type const c, OP const &&op, F1 const &&kernel, F2 &&hkernel, Args &&... args)
  {
    int SFL      = int(range.SIMDFromLower<VectorRegisterType::E_BLOCK_COUNT>());
    int SF       = int(range.SIMDFromUpper<VectorRegisterType::E_BLOCK_COUNT>());
    int ST       = int(range.SIMDToLower<VectorRegisterType::E_BLOCK_COUNT>());
    int STU      = int(range.SIMDToUpper<VectorRegisterType::E_BLOCK_COUNT>());

    std::cout << "range: (" << range.from() << ", " << range.to() << ")" << std::endl;

    VectorRegisterType         regs[sizeof...(args)];
    VectorRegisterIteratorType iters[sizeof...(args)];
    std::cout << "Vector iterator:" << std::endl;
    VectorRegisterIteratorType self_iter(this->pointer() + SFL, range.to());
    VectorRegisterType         vc(c), tmp, self;
    InitializeVectorIterators<vector_size>(range.from(), range.to(), iters, std::forward<Args>(args)...);

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
      InitializeVectorIterators<scalar_size>(range.from(), std::size_t(SF)/sizeof(type), scalar_iters, std::forward<Args>(args)...);

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
        scalar_tmp = details::MatrixReduceFreeFunction<ScalarRegisterType>::template Unroll<Args...>::Apply(
           scalar_self, scalar_regs, kernel);
        std::cout << "self = " << scalar_self << std::endl;
        std::cout << "tmp = " << scalar_tmp << std::endl;
        scalar_tmp = op(scalar_tmp, scalar_self);
        std::cout << "tmp = " << scalar_tmp << std::endl;
      }
      ret = scalar_tmp.data();
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

    if (STU != ST)
    {
      std::cout << "STU: " << STU << std::endl;
      std::cout << "ST: " << ST << std::endl;

      ScalarRegisterType         scalar_regs[sizeof...(args)];
      ScalarRegisterIteratorType scalar_iters[sizeof...(args)];
      std::cout << "Scalar iterator:" << std::endl;
      ScalarRegisterIteratorType scalar_self_iter(self_iter, 0);
      ScalarRegisterType scalar_self, scalar_tmp;
      InitializeVectorIterators<scalar_size>(ST, range.to() - ST, scalar_iters, std::forward<Args>(args)...);
      std::cout << "Scalar iterator:" << std::endl;
      std::cout << "ST for type: " << ST << std::endl;

      std::cout << "ptr: " << scalar_self_iter.pointer() << std::endl;
      std::cout << "end: " << scalar_self_iter.end() << std::endl;

      while (static_cast<void*>(scalar_self_iter.pointer()) < static_cast<void*>(self_iter.end()))
      {
        details::UnrollNext<sizeof...(args), ScalarRegisterType, ScalarRegisterIteratorType>::Apply(
          scalar_regs, scalar_iters);
        scalar_self_iter.Next(scalar_self);
        std::cout << "self = " << scalar_self << std::endl;
        scalar_tmp = details::MatrixReduceFreeFunction<ScalarRegisterType>::template Unroll<Args...>::Apply(
           scalar_self, scalar_regs, kernel);
        std::cout << "self = " << scalar_self << std::endl;
        std::cout << "tmp = " << scalar_tmp << std::endl;
        ret = op(ret, scalar_tmp.data());
        std::cout << "tmp = " << scalar_tmp << std::endl;
      }
    }

    return ret;
  }

  template <class F1, class F2, typename... Args>
  type SumReduce(F1 const &&kernel, F2 &&hkernel, Args &&... args)
  {
    Range range(0, this->size());
    return GenericRangedReduceMultiple(range, type(0), std::plus<void>{}, std::move(kernel), hkernel, std::forward<Args>(args)...);
  }

  template <class F1, class F2, typename... Args>
  type ProductReduce(F1 const &&kernel, F2 &&hkernel, Args &&... args)
  {
    Range range(0, this->size());
    return GenericRangedReduceMultiple(range, type(1), std::multiplies<void>{}, std::move(kernel), hkernel, std::forward<Args>(args)...);
  }

  template <class F1, class F2, typename... Args>
  type SumReduce(Range const &range, F1 const &&kernel, F2 &&hkernel, Args &&... args)
  {
    return GenericRangedReduceMultiple(range, type(0), std::plus<void>{}, std::move(kernel), hkernel, std::forward<Args>(args)...);
  }

  template <class F1, class F2>
  inline type Reduce(Range const &range, F1 const &&kernel, F2 &&hkernel, type const initial_value = type(0))
  {
    int SFL = int(range.SIMDFromLower<VectorRegisterType::E_BLOCK_COUNT>());
    int SF = int(range.SIMDFromUpper<VectorRegisterType::E_BLOCK_COUNT>());
    int ST = int(range.SIMDToLower<VectorRegisterType::E_BLOCK_COUNT>());
    int STU      = int(range.SIMDToUpper<VectorRegisterType::E_BLOCK_COUNT>());

    VectorRegisterType         va, vc(initial_value);
    VectorRegisterIteratorType iter(this->pointer() + SFL, range.to());
    ScalarRegisterType c(initial_value);

    if (SFL != SF)
    {
      ScalarRegisterIteratorType scalar_iter(iter, std::size_t(SF)/sizeof(type));
      ScalarRegisterType a;

      iter.Next(va);

      while (static_cast<void*>(scalar_iter.pointer()) < static_cast<void*>(iter.pointer()))
      {
        scalar_iter.Next(a);
        c = kernel(a, c);
      }
      vc = VectorRegisterType(c.data());
    }

    for (int i = SF; i < ST; i += VectorRegisterType::E_BLOCK_COUNT)
    {
      iter.Next(va);
      vc = kernel(va, vc);
    }

    c.data() = hkernel(vc);

    if (STU != ST)
    {
      ScalarRegisterIteratorType scalar_iter(iter, 0);
      ScalarRegisterType a;

      while (static_cast<void*>(scalar_iter.pointer()) < static_cast<void*>(scalar_iter.end()))
      {
        scalar_iter.Next(a);
        c = kernel(a, c);
      }
    }

    return c.data();
  }

  template <class F1, class F2>
  type Reduce(F1 const &&kernel, F2 &&hkernel, type const initial_value = type(0))
  {
    Range range(0, this->size());
    return Reduce(range, std::move(kernel), hkernel, initial_value);
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
    scalar_size = 8 * sizeof(type),
    vector_size = vectorise::VectorRegisterSize<type>::value
  };
  using ScalarRegisterType         = typename vectorise::VectorRegister<type, scalar_size>;
  using ScalarRegisterIteratorType = vectorise::VectorRegisterIterator<type, scalar_size>;
  using VectorRegisterType         = typename vectorise::VectorRegister<type, vector_size>;
  using VectorRegisterIteratorType = vectorise::VectorRegisterIterator<type, vector_size>;

  ParallelDispatcher(type *ptr, std::size_t size)
    : super_type(ptr, size)
  {}

  template <class F>
  void RangedApply(Range const &range, F const &&apply)
  {
    size_t SFL = range.SIMDFromLower<VectorRegisterType::E_BLOCK_COUNT>();
    size_t SF = range.SIMDFromUpper<VectorRegisterType::E_BLOCK_COUNT>();
    size_t ST = range.SIMDToLower<VectorRegisterType::E_BLOCK_COUNT>();
    size_t STU = range.SIMDToUpper<VectorRegisterType::E_BLOCK_COUNT>();

    VectorRegisterType vc;

    if (SFL != SF)
    {
      ScalarRegisterType  c;
      for (size_t i = range.from(); i < SF; i += ScalarRegisterType::E_BLOCK_COUNT)
      {
        apply(c);
        c.Store(this->pointer() + i);
      }
    }

    for (size_t i = SF; i < ST; i += VectorRegisterType::E_BLOCK_COUNT)
    {
      apply(vc);
      vc.Store(this->pointer() + i);
    }

    if (STU != ST)
    {
      ScalarRegisterType  c;
      for (size_t i = ST; i < range.to(); i += ScalarRegisterType::E_BLOCK_COUNT)
      {
        apply(c);
        c.Store(this->pointer() + i);
      }
    }
  }

  template <class F>
  void Apply(F const &&apply)
  {
    Range range(0, this->size());
    return RangedApply(range, std::move(apply));
  }

  template <class F, typename... Args>
  void RangedApplyMultiple(Range const &range, F const &&apply, Args &&... args)
  {
    size_t SFL = range.SIMDFromLower<VectorRegisterType::E_BLOCK_COUNT>();
    size_t SF = range.SIMDFromUpper<VectorRegisterType::E_BLOCK_COUNT>();
    size_t ST = range.SIMDToLower<VectorRegisterType::E_BLOCK_COUNT>();
    size_t STU  = range.SIMDToUpper<VectorRegisterType::E_BLOCK_COUNT>();

    VectorRegisterType         regs[sizeof...(args)], vc{type(0)};
    VectorRegisterIteratorType iters[sizeof...(args)];
    super_type::template InitializeVectorIterators<vector_size>(SF, range.to(), iters, std::forward<Args>(args)...);

    if (SFL != SF)
    {
      ScalarRegisterType  c{type(0)};
      ScalarRegisterType         scalar_regs[sizeof...(args)];
      ScalarRegisterIteratorType scalar_iters[sizeof...(args)];
      super_type::template InitializeVectorIterators<scalar_size>(range.from(), SF, scalar_iters, std::forward<Args>(args)...);

      for (size_t i = range.from(); i < SF; i += ScalarRegisterType::E_BLOCK_COUNT)
      {
        details::UnrollNext<sizeof...(args), ScalarRegisterType, ScalarRegisterIteratorType>::Apply(
          scalar_regs, scalar_iters);
        details::MatrixApplyFreeFunction<ScalarRegisterType, void>::template Unroll<Args...>::Apply(
          scalar_regs, apply, c);
        c.Store(this->pointer() + i);
      }
    }

    for (size_t i = SF; i < ST; i += VectorRegisterType::E_BLOCK_COUNT)
    {
      details::UnrollNext<sizeof...(args), VectorRegisterType, VectorRegisterIteratorType>::Apply(
          regs, iters);
      details::MatrixApplyFreeFunction<VectorRegisterType, void>::template Unroll<Args...>::Apply(
          regs, apply, vc);
      vc.Store(this->pointer() + i);
    }

    if (STU != ST)
    {
      ScalarRegisterType  c{type(0)};
      ScalarRegisterType         scalar_regs[sizeof...(args)];
      ScalarRegisterIteratorType scalar_iters[sizeof...(args)];
      super_type::template InitializeVectorIterators<scalar_size>(ST, range.to(), scalar_iters, std::forward<Args>(args)...);
      for (size_t i = ST; i < range.to(); i += ScalarRegisterType::E_BLOCK_COUNT)
      {
        details::UnrollNext<sizeof...(args), ScalarRegisterType, ScalarRegisterIteratorType>::Apply(
          scalar_regs, scalar_iters);
        details::MatrixApplyFreeFunction<ScalarRegisterType, void>::template Unroll<Args...>::Apply(
          scalar_regs, apply, c);
        c.Store(this->pointer() + i);
      }
    }
  }

  template <class F, typename... Args>
  void Apply(F const &&apply, Args &&... args)
  {
    Range range(0, this->size());
    return RangedApplyMultiple(range, std::move(apply), std::forward<Args>(args)...);
  }

  type *pointer()
  {
    return super_type::pointer();
  }
};

}  // namespace memory
}  // namespace fetch
