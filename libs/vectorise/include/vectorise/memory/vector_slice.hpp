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

#include "meta/log2.hpp"
#include "vectorise/memory/iterator.hpp"
#include "vectorise/memory/parallel_dispatcher.hpp"
#include "vectorise/platform.hpp"

#include <cstring>

namespace fetch {
namespace memory {

template <typename T, std::size_t type_size = sizeof(T)>
class VectorSlice
{
public:
  using size_type                      = std::size_t;
  using pointer_type                   = T *;
  using const_pointer_type             = T const *;
  using Type                           = T;
  using vector_slice_type              = VectorSlice;
  using iterator                       = ForwardIterator<T>;
  using reverse_iterator               = BackwardIterator<T>;
  using const_parallel_dispatcher_type = ConstParallelDispatcher<Type>;
  using parallel_dispatcher_type       = ParallelDispatcher<Type>;
  using vector_register_type           = typename parallel_dispatcher_type::vector_register_type;
  using vector_register_iterator_type =
      typename parallel_dispatcher_type::vector_register_iterator_type;

  enum
  {
    E_TYPE_SIZE     = type_size,
    E_SIMD_SIZE     = (platform::VectorRegisterSize<Type>::value >> 3),
    E_SIMD_COUNT_IM = E_SIMD_SIZE / type_size,
    E_SIMD_COUNT =
        (E_SIMD_COUNT_IM > 0 ? E_SIMD_COUNT_IM
                             : 1),  // Note that if a type is too big to fit, we pretend it can
    E_LOG_SIMD_COUNT = meta::log2(E_SIMD_COUNT),
    IS_SHARED        = 0
  };

  static_assert(E_SIMD_COUNT == (1ull << E_LOG_SIMD_COUNT), "type does not fit in SIMD");

  VectorSlice(pointer_type ptr = nullptr, std::size_t const &n = 0)
    : pointer_(ptr)
    , size_(n)
  {}

  ConstParallelDispatcher<Type> in_parallel() const
  {
    return ConstParallelDispatcher<Type>(pointer_, size());
  }
  ParallelDispatcher<Type> in_parallel()
  {
    return ParallelDispatcher<Type>(pointer(), size());
  }

  iterator begin()
  {
    return iterator(pointer_, pointer_ + size());
  }
  iterator end()
  {
    return iterator(pointer_ + size(), pointer_ + size());
  }
  reverse_iterator rbegin()
  {
    return reverse_iterator(pointer_ + size() - 1, pointer_ - 1);
  }
  reverse_iterator rend()
  {
    return reverse_iterator(pointer_ - 1, pointer_ - 1);
  }

  // TODO(private 860): ensure trivial type
  void SetAllZero()
  {
    assert(pointer_ != nullptr);
    if (pointer_)
    {
      std::memset(pointer_, 0, padded_size() * sizeof(Type));
    }
  }

  void SetPaddedZero()
  {
    assert(pointer_ != nullptr);
    if (pointer_)
    {
      std::memset(pointer_ + size(), 0, (padded_size() - size()) * sizeof(Type));
    }
  }

  void SetZeroAfter(std::size_t const &n)
  {
    if (pointer_)
    {
      std::memset(pointer_ + n, 0, (padded_size() - n) * sizeof(Type));
    }
  }

  vector_slice_type slice(std::size_t const &offset, std::size_t const &length) const
  {
    assert(std::size_t(offset / E_SIMD_COUNT) * E_SIMD_COUNT == offset);
    assert((length + offset) <= padded_size());
    return vector_slice_type(pointer_ + offset, length);
  }

  template <typename S>
  typename std::enable_if<std::is_integral<S>::value, T>::type &operator[](S const &n)
  {
    assert(pointer_ != nullptr);
    assert(std::size_t(n) < padded_size());
    return pointer_[n];
  }

  template <typename S>
  typename std::enable_if<std::is_integral<S>::value, T>::type const &operator[](S const &n) const
  {
    assert(pointer_ != nullptr);

    assert(std::size_t(n) < padded_size());
    return pointer_[n];
  }

  template <typename S>
  typename std::enable_if<std::is_integral<S>::value, T>::type &At(S const &n)
  {
    assert(pointer_ != nullptr);
    assert(n < padded_size());
    return pointer_[n];
  }

  template <typename S>
  typename std::enable_if<std::is_integral<S>::value, T>::type const &At(S const &n) const
  {
    assert(pointer_ != nullptr);
    assert(n < padded_size());
    return pointer_[n];
  }

  template <typename S>
  typename std::enable_if<std::is_integral<S>::value, T>::type const &Set(S const &n, T const &v)
  {
    assert(pointer_ != nullptr);
    assert(n < padded_size());
    pointer_[n] = v;
    return v;
  }

  std::size_t simd_size() const
  {
    return (size_) >> E_LOG_SIMD_COUNT;
  }
  std::size_t size() const
  {
    return size_;
  }

  std::size_t padded_size() const
  {
    std::size_t padded = std::size_t((size_) >> E_LOG_SIMD_COUNT) << E_LOG_SIMD_COUNT;
    if (padded < size_)
    {
      padded += E_SIMD_COUNT;
    }
    return padded;
  }

  pointer_type pointer()
  {
    return pointer_;
  }
  const_pointer_type pointer() const
  {
    return pointer_;
  }
  size_type size()
  {
    return size_;
  }

protected:
  pointer_type pointer_;
  size_type    size_;
};

}  // namespace memory
}  // namespace fetch
