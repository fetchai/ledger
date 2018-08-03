#pragma once

#include "vectorise/memory/iterator.hpp"
#include "vectorise/memory/parallel_dispatcher.hpp"
#include "vectorise/meta/log2.hpp"
#include "vectorise/platform.hpp"

#include <algorithm>
#include <atomic>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <mm_malloc.h>
#include <type_traits>

namespace fetch {
namespace memory {

template <typename T, std::size_t type_size = sizeof(T)>
class Array
{
public:
  using size_type = std::size_t;
  using data_type = T *;

  using iterator         = ForwardIterator<T>;
  using reverse_iterator = BackwardIterator<T>;
  using self_type        = Array<T>;
  using type             = T;

  using const_parallel_dispatcher_type = ConstParallelDispatcher<type>;
  using parallel_dispatcher_type       = ParallelDispatcher<type>;
  using vector_register_type           = typename parallel_dispatcher_type::vector_register_type;
  using vector_register_iterator_type =
      typename parallel_dispatcher_type::vector_register_iterator_type;

  enum
  {
    E_SIMD_SIZE     = (platform::VectorRegisterSize<type>::value >> 3),
    E_SIMD_COUNT_IM = E_SIMD_SIZE / type_size,
    E_SIMD_COUNT =
        (E_SIMD_COUNT_IM > 0 ? E_SIMD_COUNT_IM
                             : 1),  // Note that if a type is too big to fit, we pretend it can
    E_LOG_SIMD_COUNT = fetch::meta::Log2<E_SIMD_COUNT>::value,
    IS_SHARED        = 0
  };

  static_assert(E_SIMD_COUNT == (1ull << E_LOG_SIMD_COUNT), "type does not fit in SIMD");

  Array(std::size_t const &n)
  {
    size_ = n;
    if (n > 0) data_ = (type *)_mm_malloc(padded_size() * sizeof(type), 16);
  }

  Array() : Array(0) {}

  Array(Array const &other) { this->operator=(other); }

  Array(Array &&other)
  {
    std::swap(size_, other.size_);
    std::swap(data_, other.data_);
  }

  ConstParallelDispatcher<type> in_parallel() const
  {
    return ConstParallelDispatcher<type>(pointer(), size());
  }
  ParallelDispatcher<type> in_parallel() { return ParallelDispatcher<type>(pointer(), size()); }

  void SetAllZero()
  {
    assert(data_ != nullptr);

    memset(data_, 0, padded_size() * sizeof(type));
  }

  void SetPaddedZero()
  {
    assert(data_ != nullptr);

    memset(data_ + size(), 0, (padded_size() - size()) * sizeof(type));
  }

  void SetZeroAfter(std::size_t const &n)
  {
    memset(data_ + n, 0, (padded_size() - n) * sizeof(type));
  }

  Array &operator=(Array &&other)
  {
    std::swap(size_, other.size_);
    std::swap(data_, other.data_);
    return *this;
  }

  ~Array()
  {
    if (data_ != nullptr) _mm_free(data_);
  }

  Array<type> Copy() const
  {
    Array<type> ret(size_);
    for (std::size_t i = 0; i < size_; ++i)
    {
      ret[i] = At(i);
    }

    return ret;
  }

  iterator         begin() { return iterator(data_, data_ + size()); }
  iterator         end() { return iterator(data_ + size(), data_ + size()); }
  reverse_iterator rbegin() { return reverse_iterator(data_ + size() - 1, data_ - 1); }
  reverse_iterator rend() { return reverse_iterator(data_ - 1, data_ - 1); }

  T &operator[](std::size_t const &n)
  {
    assert(data_ != nullptr);
    assert(n < padded_size());
    return data_[n];
  }

  T const &operator[](std::size_t const &n) const
  {
    assert(data_ != nullptr);

    assert(n < padded_size());
    return data_[n];
  }

  T &At(std::size_t const &n)
  {
    assert(data_ != nullptr);
    assert(n < padded_size());
    return data_[n];
  }

  T const &At(std::size_t const &n) const
  {
    assert(data_ != nullptr);
    assert(n < padded_size());
    return data_[n];
  }

  T const &Set(std::size_t const &n, T const &v)
  {
    assert(data_ != nullptr);
    assert(n < padded_size());
    data_[n] = v;
    return v;
  }

  self_type &operator=(Array const &other)
  {
    if (&other == this) return *this;

    if (data_ != nullptr) free(data_);
    data_ = (type *)_mm_malloc(other.padded_size() * sizeof(type), 16);
    size_ = other.size();

    for (std::size_t i = 0; i < size_; ++i) data_[i] = other[i];

    return *this;
  }

  std::size_t simd_size() const { return (size_) >> E_LOG_SIMD_COUNT; }

  std::size_t size() const { return size_; }

  std::size_t padded_size() const
  {
    std::size_t padded = std::size_t((size_) >> E_LOG_SIMD_COUNT) << E_LOG_SIMD_COUNT;
    if (padded < size_) padded += E_SIMD_COUNT;
    return padded;
  }

  type *pointer() const { return data_; }

private:
  size_type size_ = 0;
  T *       data_ = nullptr;
};
}  // namespace memory
}  // namespace fetch
