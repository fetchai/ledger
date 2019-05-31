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

#include <algorithm>
#include <cstring>

namespace fetch {
namespace memory {

template <class T, bool = meta::IsPOD<T>>
struct ZeroMem
{
  static constexpr void Call(T *buf, std::size_t size)
  {
    std::fill(buf, buf + size, T{});
  }
};

template <class T>
struct ZeroMem<T, true>
{
  static constexpr void Call(T *buf, std::size_t size)
  {
    std::memset(static_cast<void *>(buf), 0, size * sizeof(T));
  }
};

template <typename T, std::size_t type_size = sizeof(T)>
class VectorSlice
{
public:
  using SizeType                    = std::size_t;
  using PointerType                 = T *;
  using ConstPointerType            = T const *;
  using Type                        = T;
  using VectorSliceType             = VectorSlice;
  using Iterator                    = ForwardIterator<T>;
  using ReverseIterator             = BackwardIterator<T>;
  using ConstParallelDispatcherType = ConstParallelDispatcher<Type>;
  using ParallelDispatcherType      = ParallelDispatcher<Type>;
  using VectorRegisterType          = typename ParallelDispatcherType::VectorRegisterType;
  using VectorRegisterIteratorType  = typename ParallelDispatcherType::VectorRegisterIteratorType;

  enum
  {
    E_TYPE_SIZE     = type_size,
    E_SIMD_SIZE     = (platform::VectorRegisterSize<Type>::value >> 3),
    E_SIMD_COUNT_IM = E_SIMD_SIZE / type_size,
    E_SIMD_COUNT =
        (E_SIMD_COUNT_IM > 0 ? E_SIMD_COUNT_IM
                             : 1),  // Note that if a type is too big to fit, we pretend it can
    E_LOG_SIMD_COUNT = meta::Log2(E_SIMD_COUNT),
    IS_SHARED        = 0
  };

  static_assert(E_SIMD_COUNT == (1ull << E_LOG_SIMD_COUNT), "type does not fit in SIMD");

  constexpr VectorSlice(PointerType ptr = nullptr, std::size_t n = 0) noexcept
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

  constexpr Iterator begin() noexcept
  {
    return Iterator{pointer_, pointer_ + size()};
  }
  constexpr Iterator end() noexcept
  {
    return Iterator{pointer_ + size(), pointer_ + size()};
  }
  constexpr ReverseIterator rbegin() noexcept
  {
    return ReverseIterator(pointer_ + size() - 1, pointer_ - 1);
  }
  constexpr ReverseIterator rend() noexcept
  {
    return ReverseIterator(pointer_ - 1, pointer_ - 1);
  }

  // TODO(private 860): ensure trivial type
  void SetAllZero()
  {
    if (pointer_)
    {
      ZeroMem<T>::Call(pointer_, padded_size());
    }
  }

  void SetPaddedZero()
  {
    if (pointer_)
    {
      ZeroMem<T>::Call(pointer_ + size(), padded_size() - size());
    }
  }

  void SetZeroAfter(std::size_t n)
  {
    if (pointer_)
    {
      ZeroMem<T>::Call(pointer_ + n, padded_size() - n);
    }
  }

  constexpr VectorSliceType slice(std::size_t offset, std::size_t length) const noexcept
  {
    assert(std::size_t(offset / E_SIMD_COUNT) * E_SIMD_COUNT == offset);
    assert((length + offset) <= padded_size());
    return VectorSliceType(pointer_ + offset, length);
  }

  template <typename S>
  constexpr typename std::enable_if<std::is_integral<S>::value, T>::type &operator[](
      S const &n) noexcept
  {
    assert(pointer_ != nullptr);
    assert(std::size_t(n) < padded_size());
    return pointer_[n];
  }

  template <typename S>
  constexpr typename std::enable_if<std::is_integral<S>::value, T>::type const &operator[](
      S const &n) const noexcept
  {
    assert(pointer_ != nullptr);

    assert(std::size_t(n) < padded_size());
    return pointer_[n];
  }

  template <typename S>
  constexpr typename std::enable_if<std::is_integral<S>::value, T>::type &At(S const &n) noexcept
  {
    assert(pointer_ != nullptr);
    assert(n < padded_size());
    return pointer_[n];
  }

  template <typename S>
  constexpr typename std::enable_if<std::is_integral<S>::value, T>::type const &At(S const &n) const
      noexcept
  {
    assert(pointer_ != nullptr);
    assert(n < padded_size());
    return pointer_[n];
  }

  template <typename S>
  constexpr typename std::enable_if<std::is_integral<S>::value, T>::type const &Set(
      S const &n, T const &v) noexcept
  {
    assert(pointer_ != nullptr);
    assert(n < padded_size());
    pointer_[n] = v;
    return v;
  }

  constexpr SizeType simd_size() const noexcept
  {
    return (size_) >> E_LOG_SIMD_COUNT;
  }

  constexpr SizeType const &size() const noexcept
  {
    return size_;
  }

  constexpr SizeType padded_size() const noexcept
  {
    std::size_t padded = std::size_t((size_) >> E_LOG_SIMD_COUNT) << E_LOG_SIMD_COUNT;
    if (padded < size_)
    {
      padded += E_SIMD_COUNT;
    }
    return padded;
  }

  constexpr PointerType pointer() noexcept
  {
    return pointer_;
  }

  constexpr ConstPointerType pointer() const noexcept
  {
    return pointer_;
  }

  constexpr SizeType const &size() noexcept
  {
    return size_;
  }

protected:
  PointerType pointer_;
  SizeType    size_;
};

}  // namespace memory
}  // namespace fetch
