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
    vector_size = platform::VectorRegisterSize<type>::value
  };
  using vector_register_type          = typename vectorize::VectorRegister<type, vector_size>;
  using vector_register_iterator_type = vectorize::VectorRegisterIterator<type, vector_size>;

  ConstParallelDispatcher(type *ptr, std::size_t const &size)
    : pointer_(ptr)
    , size_(size)
  {}

  type Reduce(vector_register_type (*vector_reduction)(vector_register_type const &,
                                                       vector_register_type const &)) const
  {
    vector_register_type          a, b(type(0));
    vector_register_iterator_type iter(this->pointer(), this->size());

    std::size_t N = this->size();

    for (std::size_t i = 0; i < N; i += vector_register_type::E_BLOCK_COUNT)
    {
      iter.Next(a);
      b = vector_reduction(a, b);
    }

    return reduce(b);
  }

  /// Sum reduce
  /// @{
  template <typename F, typename V>
  type SumReduce(F &&vector_reduce, V const &a, V const &b, V const &c)
  {
    vector_register_iterator_type self_iter(this->pointer(), this->size());
    vector_register_iterator_type a_iter(a.pointer(), a.size());
    vector_register_iterator_type b_iter(b.pointer(), b.size());
    vector_register_iterator_type c_iter(c.pointer(), c.size());

    vector_register_type ret(type(0)), tmp, self;

    vector_register_type a_val;
    vector_register_type b_val;
    vector_register_type c_val;

    std::size_t N = this->size();
    for (std::size_t i = 0; i < N; i += vector_register_type::E_BLOCK_COUNT)
    {
      self_iter.Next(self);
      a_iter.Next(a_val);
      b_iter.Next(b_val);
      c_iter.Next(c_val);
      tmp = vector_reduce(self, a_val, b_val, c_val);
      ret = ret + tmp;
    }

    return reduce(c);
  }

  template <typename F, typename V>
  type SumReduce(F &&vector_reduce, V const &a, V const &b)
  {
    vector_register_iterator_type self_iter(this->pointer(), this->size());
    vector_register_iterator_type a_iter(a.pointer(), a.size());
    vector_register_iterator_type b_iter(b.pointer(), b.size());

    vector_register_type c(type(0)), tmp, self;

    vector_register_type a_val;
    vector_register_type b_val;

    std::size_t N = this->size();
    for (std::size_t i = 0; i < N; i += vector_register_type::E_BLOCK_COUNT)
    {
      self_iter.Next(self);
      a_iter.Next(a_val);
      b_iter.Next(b_val);
      tmp = vector_reduce(self, a_val, b_val);
      c   = c + tmp;
    }

    return reduce(c);
  }

  template <typename F, typename V>
  typename std::enable_if<!std::is_same<F, TrivialRange>::value, type>::type SumReduce(
      F &&vector_reduce, V const &a)
  {
    vector_register_iterator_type self_iter(this->pointer(), this->size());
    vector_register_iterator_type a_iter(a.pointer(), a.size());

    vector_register_type c(type(0)), tmp, self;

    vector_register_type a_val;

    std::size_t N = this->size();
    for (std::size_t i = 0; i < N; i += vector_register_type::E_BLOCK_COUNT)
    {
      self_iter.Next(self);
      a_iter.Next(a_val);
      tmp = vector_reduce(self, a_val);
      c   = c + tmp;
    }

    return reduce(c);
  }

  template <typename F>
  type SumReduce(F &&vector_reduce)
  {
    vector_register_iterator_type self_iter(this->pointer(), this->size());
    vector_register_type          c(type(0)), tmp, self;

    std::size_t N = this->size();
    for (std::size_t i = 0; i < N; i += vector_register_type::E_BLOCK_COUNT)
    {
      self_iter.Next(self);
      tmp = vector_reduce(self);
      c   = c + tmp;
    }

    return reduce(c);
  }
  ///  @}

  /// SumReduce with range
  /// @{

  template <typename F, typename V>
  type SumReduce(TrivialRange const &range, F &&vector_reduce, V const &a, V const &b, V const &c)
  {
    int SFL      = int(range.SIMDFromLower<vector_register_type::E_BLOCK_COUNT>());
    int SF       = int(range.SIMDFromUpper<vector_register_type::E_BLOCK_COUNT>());
    int ST       = int(range.SIMDToLower<vector_register_type::E_BLOCK_COUNT>());
    int STU      = int(range.SIMDToUpper<vector_register_type::E_BLOCK_COUNT>());
    int SIMDSize = STU - SFL;

    vector_register_iterator_type self_iter(this->pointer(), std::size_t(SIMDSize));
    vector_register_iterator_type a_iter(a.pointer(), std::size_t(SIMDSize));
    vector_register_iterator_type b_iter(b.pointer(), std::size_t(SIMDSize));
    vector_register_iterator_type c_iter(c.pointer(), std::size_t(SIMDSize));

    vector_register_type vec_ret(type(0)), tmp, self;

    vector_register_type a_val;
    vector_register_type b_val;
    vector_register_type c_val;

    // Taking care of thread
    type ret = 0;
    if (SFL != SF)
    {
      self_iter.Next(self);
      a_iter.Next(a_val);
      b_iter.Next(b_val);
      c_iter.Next(c_val);
      tmp = vector_reduce(self, a_val, b_val, c_val);

      int Q = vector_register_type::E_BLOCK_COUNT - (SF - int(range.from()));
      for (int i = 0; i < vector_register_type::E_BLOCK_COUNT; ++i)
      {
        if (Q <= i)
        {
          ret += first_element(tmp);
        }
        tmp = shift_elements_right(tmp);
      }
    }

    // Mid
    for (int i = SF; i < ST; i += vector_register_type::E_BLOCK_COUNT)
    {
      self_iter.Next(self);
      a_iter.Next(a_val);
      b_iter.Next(b_val);
      c_iter.Next(c_val);
      tmp     = vector_reduce(self, a_val, b_val, c_val);
      vec_ret = vec_ret + tmp;
    }

    ret += reduce(vec_ret);

    // Taking care of the tail
    if (STU != ST)
    {
      self_iter.Next(self);
      a_iter.Next(a_val);
      b_iter.Next(b_val);
      c_iter.Next(c_val);
      tmp = vector_reduce(self, a_val, b_val, c_val);

      int Q = (int(range.to()) - ST - 1);
      for (int i = 0; i <= Q; ++i)
      {
        ret += first_element(tmp);
        tmp = shift_elements_right(tmp);
      }
    }

    return ret;
  }

  template <typename F, typename V>
  type SumReduce(TrivialRange const &range, F &&vector_reduce, V const &a, V const &b)
  {

    vector_register_type c(type(0)), tmp, self;

    vector_register_type a_val;
    vector_register_type b_val;

    int SFL      = int(range.SIMDFromLower<vector_register_type::E_BLOCK_COUNT>());
    int SF       = int(range.SIMDFromUpper<vector_register_type::E_BLOCK_COUNT>());
    int ST       = int(range.SIMDToLower<vector_register_type::E_BLOCK_COUNT>());
    int STU      = int(range.SIMDToUpper<vector_register_type::E_BLOCK_COUNT>());
    int SIMDSize = STU - SFL;

    vector_register_iterator_type self_iter(this->pointer(), std::size_t(SIMDSize));
    vector_register_iterator_type a_iter(a.pointer(), std::size_t(SIMDSize));
    vector_register_iterator_type b_iter(b.pointer(), std::size_t(SIMDSize));

    // Taking care of thread
    type ret = 0;
    if (SFL != SF)
    {
      self_iter.Next(self);
      a_iter.Next(a_val);
      b_iter.Next(b_val);
      tmp = vector_reduce(self, a_val, b_val);

      int Q = vector_register_type::E_BLOCK_COUNT - (SF - int(range.from()));
      for (int i = 0; i < vector_register_type::E_BLOCK_COUNT; ++i)
      {
        if (Q <= i)
        {
          ret += first_element(tmp);
        }
        tmp = shift_elements_right(tmp);
      }
    }

    // Mid
    for (int i = SF; i < ST; i += vector_register_type::E_BLOCK_COUNT)
    {
      self_iter.Next(self);
      a_iter.Next(a_val);
      b_iter.Next(b_val);
      tmp = vector_reduce(self, a_val, b_val);
      c   = c + tmp;
    }

    ret += reduce(c);

    // Taking care of the tail
    if (STU != ST)
    {
      self_iter.Next(self);
      a_iter.Next(a_val);
      b_iter.Next(b_val);
      tmp = vector_reduce(self, a_val, b_val);

      int Q = (int(range.to()) - ST - 1);
      for (int i = 0; i <= Q; ++i)
      {
        ret += first_element(tmp);
        tmp = shift_elements_right(tmp);
      }
    }

    return ret;
  }

  template <typename F, typename V>
  type SumReduce(TrivialRange const &range, F &&vector_reduce, V const &a)
  {
    int SFL      = int(range.SIMDFromLower<vector_register_type::E_BLOCK_COUNT>());
    int SF       = int(range.SIMDFromUpper<vector_register_type::E_BLOCK_COUNT>());
    int ST       = int(range.SIMDToLower<vector_register_type::E_BLOCK_COUNT>());
    int STU      = int(range.SIMDToUpper<vector_register_type::E_BLOCK_COUNT>());
    int SIMDSize = STU - SFL;

    vector_register_iterator_type self_iter(this->pointer(), std::size_t(SIMDSize));
    vector_register_iterator_type a_iter(a.pointer(), std::size_t(SIMDSize));

    vector_register_type c(type(0)), tmp, self;

    vector_register_type a_val;

    // Taking care of thread
    type ret = 0;
    if (SFL != SF)
    {
      self_iter.Next(self);
      a_iter.Next(a_val);
      tmp = vector_reduce(self, a_val);

      int Q = vector_register_type::E_BLOCK_COUNT - (SF - int(range.from()));
      for (int i = 0; i < vector_register_type::E_BLOCK_COUNT; ++i)
      {
        if (Q <= i)
        {
          ret += first_element(tmp);
        }
        tmp = shift_elements_right(tmp);
      }
    }

    for (int i = SF; i < ST; i += vector_register_type::E_BLOCK_COUNT)
    {
      self_iter.Next(self);
      a_iter.Next(a_val);
      tmp = vector_reduce(self, a_val);
      c   = c + tmp;
    }

    ret += reduce(c);

    // Taking care of the tail
    if (STU != ST)
    {
      self_iter.Next(self);
      a_iter.Next(a_val);
      tmp = vector_reduce(self, a_val);

      int Q = (int(range.to()) - ST - 1);
      for (int i = 0; i <= Q; ++i)
      {
        ret += first_element(tmp);
        tmp = shift_elements_right(tmp);
      }
    }

    return ret;
  }

  template <typename F>
  type SumReduce(TrivialRange const &range, F &&vector_reduce)
  {
    int SFL      = int(range.SIMDFromLower<vector_register_type::E_BLOCK_COUNT>());
    int SF       = int(range.SIMDFromUpper<vector_register_type::E_BLOCK_COUNT>());
    int ST       = int(range.SIMDToLower<vector_register_type::E_BLOCK_COUNT>());
    int STU      = int(range.SIMDToUpper<vector_register_type::E_BLOCK_COUNT>());
    int SIMDSize = STU - SFL;

    type                          ret = 0;
    vector_register_iterator_type self_iter(this->pointer(), std::size_t(SIMDSize));
    vector_register_type          c(type(0)), tmp, self;

    // Taking care of thread
    if (SFL != SF)
    {
      self_iter.Next(self);
      tmp = vector_reduce(self);

      int Q = vector_register_type::E_BLOCK_COUNT - (SF - int(range.from()));
      for (int i = 0; i < vector_register_type::E_BLOCK_COUNT; ++i)
      {
        if (Q <= i)
        {
          ret += first_element(tmp);
        }
        tmp = shift_elements_right(tmp);
      }
    }

    for (int i = SF; i < ST; i += vector_register_type::E_BLOCK_COUNT)
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
  /// @}

  template <typename... Args>
  type ProductReduce(typename details::MatrixReduceFreeFunction<vector_register_type>::
                         template Unroll<Args...>::signature_type const &kernel,
                     Args &&... args)
  {

    vector_register_type          regs[sizeof...(args)];
    vector_register_iterator_type iters[sizeof...(args)];
    InitializeVectorIterators(0, this->size(), iters, std::forward<Args>(args)...);

    vector_register_iterator_type self_iter(this->pointer(), this->size());
    vector_register_type          c(type(0)), tmp, self;

    std::size_t N = this->size();
    for (std::size_t i = 0; i < N; i += vector_register_type::E_BLOCK_COUNT)
    {
      details::UnrollNext<sizeof...(args), vector_register_type,
                          vector_register_iterator_type>::Apply(regs, iters);
      self_iter.Next(self);
      tmp =
          details::MatrixReduceFreeFunction<vector_register_type>::template Unroll<Args...>::Apply(
              self, regs, kernel);
      c = c * tmp;
    }

    return reduce(c);
  }

  type Reduce(TrivialRange const &range,
              vector_register_type (*vector_reduction)(vector_register_type const &,
                                                       vector_register_type const &)) const
  {

    int SFL = int(range.SIMDFromLower<vector_register_type::E_BLOCK_COUNT>());

    int SF = int(range.SIMDFromUpper<vector_register_type::E_BLOCK_COUNT>());
    int ST = int(range.SIMDToLower<vector_register_type::E_BLOCK_COUNT>());

    int STU      = int(range.SIMDToUpper<vector_register_type::E_BLOCK_COUNT>());
    int SIMDSize = STU - SFL;

    vector_register_type          a, b(type(0));
    vector_register_iterator_type iter(this->pointer() + SFL, std::size_t(SIMDSize));

    if (SFL != SF)
    {
      iter.Next(a);
      a = vector_zero_below_element(a,
                                    vector_register_type::E_BLOCK_COUNT - (SF - int(range.from())));
      b = vector_reduction(a, b);
    }

    for (int i = SF; i < ST; i += vector_register_type::E_BLOCK_COUNT)
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
    for (std::size_t i = 0; i < vector_register_type::E_BLOCK_COUNT; ++i)
    {
      vector_register_type c(ret);
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

  type Reduce(TrivialRange const &range,
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
  std::size_t const &size() const
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
  static void InitializeVectorIterators(std::size_t const &offset, std::size_t const &size,
                                        vector_register_iterator_type *iters, G &next,
                                        Args &&... remaining)
  {

    assert(next.padded_size() >= offset + size);
    (*iters) = vector_register_iterator_type(next.pointer() + offset, size);
    InitializeVectorIterators(offset, size, iters + 1, std::forward<Args>(remaining)...);
  }

  template <typename G>
  static void InitializeVectorIterators(std::size_t const &offset, std::size_t const &size,
                                        vector_register_iterator_type *iters, G &next)
  {
    assert(next.padded_size() >= offset + size);
    (*iters) = vector_register_iterator_type(next.pointer() + offset, size);
  }

  static void InitializeVectorIterators(std::size_t const & /*offset*/,
                                        std::size_t const & /*size*/,
                                        vector_register_iterator_type * /*iters*/)
  {}

  template <typename G, typename... Args>
  static void SetPointers(std::size_t const &offset, std::size_t const &size, type const **regs,
                          G &next, Args &&... remaining)
  {

    assert(next.size() >= offset + size);
    *regs = next.pointer() + offset;
    SetPointers(offset, size, regs + 1, std::forward<Args>(remaining)...);
  }

  template <typename G>
  static void SetPointers(std::size_t const &offset, std::size_t const & /*size*/,
                          type const **regs, G &next)
  {
    // assert(next.size() >= offset + size); // Size not used
    *regs = next.pointer() + offset;
  }

  static void SetPointers(std::size_t const & /*offset*/, std::size_t const & /*size*/,
                          vector_register_iterator_type * /*iters*/)
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
    vector_size = platform::VectorRegisterSize<type>::value
  };
  using vector_register_type          = typename vectorize::VectorRegister<type, vector_size>;
  using vector_register_iterator_type = vectorize::VectorRegisterIterator<type, vector_size>;

  ParallelDispatcher(type *ptr, std::size_t const &size)
    : super_type(ptr, size)
  {}

  template <typename F>
  void Apply(F &&apply)
  {
    vector_register_type c;

    std::size_t N = this->size();
    for (std::size_t i = 0; i < N; i += vector_register_type::E_BLOCK_COUNT)
    {
      assert(i < this->size());
      apply(c);

      c.Store(this->pointer() + i);
    }
  }

  template <typename... Args>
  void Apply(typename details::MatrixApplyFreeFunction<vector_register_type, void>::template Unroll<
                 Args...>::signature_type &&apply,
             Args &&... args)
  {
    std::cout << "Was here?? " << std::endl;

    vector_register_type          regs[sizeof...(args)], c;
    vector_register_iterator_type iters[sizeof...(args)];
    ConstParallelDispatcher<T>::InitializeVectorIterators(0, this->size(), iters,
                                                          std::forward<Args>(args)...);

    std::size_t N = this->size();
    for (std::size_t i = 0; i < N; i += vector_register_type::E_BLOCK_COUNT)
    {
      details::UnrollNext<sizeof...(args), vector_register_type,
                          vector_register_iterator_type>::Apply(regs, iters);

      details::MatrixApplyFreeFunction<vector_register_type, void>::template Unroll<Args...>::Apply(
          regs, std::move(apply), c);

      c.Store(this->pointer() + i);
    }
  }

  template <typename F>
  void Apply(TrivialRange const &range, F &&apply)
  {

    int SFL = int(range.SIMDFromLower<vector_register_type::E_BLOCK_COUNT>());

    int SF = int(range.SIMDFromUpper<vector_register_type::E_BLOCK_COUNT>());
    int ST = int(range.SIMDToLower<vector_register_type::E_BLOCK_COUNT>());

    int STU = int(range.SIMDToUpper<vector_register_type::E_BLOCK_COUNT>());

    vector_register_type c;

    if (SFL != SF)
    {
      apply(c);

      int Q = vector_register_type::E_BLOCK_COUNT - (SF - int(range.from()));
      for (int i = 0; i < vector_register_type::E_BLOCK_COUNT; ++i)
      {
        type value = first_element(c);
        c          = shift_elements_right(c);
        if (Q <= i)
        {
          this->pointer()[SFL + i] = value;
        }
      }
    }

    for (int i = SF; i < ST; i += vector_register_type::E_BLOCK_COUNT)
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

  template <typename... Args>
  void Apply(TrivialRange const &range,
             typename details::MatrixApplyFreeFunction<vector_register_type, void>::template Unroll<
                 Args...>::signature_type const &apply,
             Args &&... args)
  {

    int SFL = int(range.SIMDFromLower<vector_register_type::E_BLOCK_COUNT>());

    int SF = int(range.SIMDFromUpper<vector_register_type::E_BLOCK_COUNT>());
    int ST = int(range.SIMDToLower<vector_register_type::E_BLOCK_COUNT>());

    int STU      = int(range.SIMDToUpper<vector_register_type::E_BLOCK_COUNT>());
    int SIMDSize = STU - SFL;

    vector_register_type          regs[sizeof...(args)], c;
    vector_register_iterator_type iters[sizeof...(args)];

    ConstParallelDispatcher<T>::InitializeVectorIterators(std::size_t(SFL), std::size_t(SIMDSize),
                                                          iters, std::forward<Args>(args)...);

    if (SFL != SF)
    {
      details::UnrollNext<sizeof...(args), vector_register_type,
                          vector_register_iterator_type>::Apply(regs, iters);
      details::MatrixApplyFreeFunction<vector_register_type, void>::template Unroll<Args...>::Apply(
          regs, apply, c);

      int Q = vector_register_type::E_BLOCK_COUNT - (SF - int(range.from()));
      for (int i = 0; i < vector_register_type::E_BLOCK_COUNT; ++i)
      {
        type value = first_element(c);
        c          = shift_elements_right(c);
        if (Q <= i)
        {

          this->pointer()[SFL + i] = value;
        }
      }
    }

    for (int i = SF; i < ST; i += vector_register_type::E_BLOCK_COUNT)
    {
      details::UnrollNext<sizeof...(args), vector_register_type,
                          vector_register_iterator_type>::Apply(regs, iters);
      details::MatrixApplyFreeFunction<vector_register_type, void>::template Unroll<Args...>::Apply(
          regs, apply, c);

      c.Store(this->pointer() + i);
    }

    if (STU != ST)
    {
      details::UnrollNext<sizeof...(args), vector_register_type,
                          vector_register_iterator_type>::Apply(regs, iters);
      details::MatrixApplyFreeFunction<vector_register_type, void>::template Unroll<Args...>::Apply(
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

  template <class C, typename... Args>
  void Apply(C const &cls,
             typename details::MatrixApplyClassMember<C, vector_register_type, void>::
                 template Unroll<Args...>::signature_type const &fnc,
             Args &&... args)
  {

    vector_register_type          regs[sizeof...(args)];
    vector_register_type          c;
    vector_register_iterator_type iters[sizeof...(args)];
    std::size_t                   N = super_type::size();
    ConstParallelDispatcher<T>::InitializeVectorIterators(0, N, iters, std::forward<Args>(args)...);

    for (std::size_t i = 0; i < N; i += vector_register_type::E_BLOCK_COUNT)
    {
      //      assert(i < N);

      details::UnrollNext<sizeof...(args), vector_register_type,
                          vector_register_iterator_type>::Apply(regs, iters);

      details::MatrixApplyClassMember<C, vector_register_type,
                                      void>::template Unroll<Args...>::Apply(regs, cls, fnc, c);
      c.Store(this->pointer() + i);
    }
  }

  template <class C, typename... Args>
  void Apply(C const &cls,
             typename details::MatrixApplyClassMember<C, type, void>::template Unroll<
                 Args...>::signature_type const &fnc,
             Args &&... args)
  {

    constexpr std::size_t R = sizeof...(args);
    type const *          regs[R];
    type                  c;

    std::size_t N = super_type::size();

    ConstParallelDispatcher<T>::SetPointers(0, N, regs, std::forward<Args>(args)...);

    for (std::size_t i = 0; i < N; ++i)
    {

      details::MatrixApplyClassMember<C, type, void>::template Unroll<Args...>::Apply(regs, cls,
                                                                                      fnc, c);

      this->pointer()[i] = c;

      for (std::size_t j = 0; j < R; ++j)
      {
        ++regs[j];
      }
    }
  }

  template <class C, typename... Args>
  typename std::enable_if<std::is_same<decltype(&C::operator()),
                                       typename details::MatrixApplyClassMember<C, type, void>::
                                           template Unroll<Args...>::signature_type>::value,
                          void>::type
  Apply(C const &cls, Args &&... args)
  {
    return Apply(cls, &C::operator(), std::forward<Args>(args)...);
  }

  template <class C, typename... Args>
  typename std::enable_if<
      std::is_same<decltype(&C::operator()),
                   typename details::MatrixApplyClassMember<C, vector_register_type, void>::
                       template Unroll<Args...>::signature_type>::value,
      void>::type
  Apply(C const &cls, Args &&... args)
  {

    return Apply(cls, &C::operator(), std::forward<Args>(args)...);
  }

  template <class C, typename... Args>
  void Apply(TrivialRange const &range, C const &cls,
             typename details::MatrixApplyClassMember<
                 C, vector_register_type, void>::template Unroll<Args...>::signature_type &fnc,
             Args &&... args)
  {

    int SFL = int(range.SIMDFromLower<vector_register_type::E_BLOCK_COUNT>());

    int SF = int(range.SIMDFromUpper<vector_register_type::E_BLOCK_COUNT>());
    int ST = int(range.SIMDToLower<vector_register_type::E_BLOCK_COUNT>());

    int STU      = int(range.SIMDToUpper<vector_register_type::E_BLOCK_COUNT>());
    int SIMDSize = STU - SFL;

    vector_register_type          regs[sizeof...(args)], c;
    vector_register_iterator_type iters[sizeof...(args)];
    ConstParallelDispatcher<T>::InitializeVectorIterators(std::size_t(SFL), std::size_t(SIMDSize),
                                                          iters, std::forward<Args>(args)...);

    if (SFL != SF)
    {
      details::UnrollNext<sizeof...(args), vector_register_type,
                          vector_register_iterator_type>::Apply(regs, iters);
      details::MatrixApplyClassMember<C, vector_register_type,
                                      void>::template Unroll<Args...>::Apply(regs, cls, fnc, c);

      int Q = vector_register_type::E_BLOCK_COUNT - (SF - int(range.from()));
      for (int i = 0; i < vector_register_type::E_BLOCK_COUNT; ++i)
      {
        type value = first_element(c);
        c          = shift_elements_right(c);
        if (Q <= i)
        {
          this->pointer()[SFL + i] = value;
        }
      }
    }

    for (int i = SF; i < ST; i += vector_register_type::E_BLOCK_COUNT)
    {
      details::UnrollNext<sizeof...(args), vector_register_type,
                          vector_register_iterator_type>::Apply(regs, iters);
      details::MatrixApplyClassMember<C, vector_register_type,
                                      void>::template Unroll<Args...>::Apply(regs, cls, fnc, c);
      c.Store(this->pointer() + i);
    }

    if (STU != ST)
    {
      details::UnrollNext<sizeof...(args), vector_register_type,
                          vector_register_iterator_type>::Apply(regs, iters);
      details::MatrixApplyClassMember<C, vector_register_type,
                                      void>::template Unroll<Args...>::Apply(regs, cls, fnc, c);

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

private:
};

}  // namespace memory
}  // namespace fetch
