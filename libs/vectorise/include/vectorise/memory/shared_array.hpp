#pragma once

#include "vectorise/memory/iterator.hpp"
#include "vectorise/memory/parallel_dispatcher.hpp"
#include "vectorise/memory/vector_slice.hpp"
#include "vectorise/meta/log2.hpp"

#include <algorithm>
#include <atomic>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <memory>
#include <mm_malloc.h>
#include <stdlib.h>
#include <type_traits>
namespace fetch {
namespace memory {

template <typename T, std::size_t type_size = sizeof(T)>
class SharedArray : public VectorSlice<T, type_size>
{
public:
  typedef std::size_t               size_type;
  typedef std::shared_ptr<T>        data_type;
  typedef VectorSlice<T, type_size> super_type;
  typedef SharedArray               self_type;
  typedef T                         type;

  SharedArray(std::size_t const &n) : super_type()
  {
    this->size_ = n;

    if (n > 0)
    {
      data_ =
          std::shared_ptr<T>((type *)_mm_malloc(this->padded_size() * sizeof(type), 16), _mm_free);
      this->pointer_ = data_.get();
    }
  }

  SharedArray() {}
  SharedArray(SharedArray const &other)
    : super_type(other.data_.get(), other.size()), data_(other.data_)
  {}

  SharedArray(SharedArray &&other)
  {
    std::swap(this->size_, other.size_);
    std::swap(this->data_, other.data_);
    std::swap(this->pointer_, other.pointer_);
  }

  SharedArray &operator=(SharedArray &&other)
  {
    std::swap(this->size_, other.size_);
    std::swap(this->data_, other.data_);
    std::swap(this->pointer_, other.pointer_);
    return *this;
  }
  self_type &operator=(SharedArray const &other)
  {
    if (&other == this) return *this;

    this->size_    = other.size_;
    this->data_    = other.data_;
    this->pointer_ = other.pointer_;
    return *this;
  }

  ~SharedArray() {}

  self_type Copy() const
  {
    // TODO: Use memcopy
    self_type ret(this->size_);
    for (std::size_t i = 0; i < this->size_; ++i)
    {
      ret[i] = this->At(i);
    }

    return ret;
  }

private:
  data_type data_ = nullptr;
};

/*
template <typename T, std::size_t type_size = sizeof(T)>
class SharedArray {
public:
typedef std::size_t size_type;
typedef std::shared_ptr<T> data_type;

typedef ForwardIterator<T> iterator;
typedef BackwardIterator<T> reverse_iterator;
typedef SharedArray<T, type_size> self_type;
typedef T type;

typedef ConstParallelDispatcher<type> const_parallel_dispatcher_type;
typedef ParallelDispatcher<type> parallel_dispatcher_type;
typedef typename parallel_dispatcher_type::vector_register_type
vector_register_type; typedef typename
parallel_dispatcher_type::vector_register_iterator_type
vector_register_iterator_type;


enum {
  E_SIMD_SIZE =  (platform::VectorRegisterSize<T>::value >> 3),
  E_SIMD_COUNT_IM = E_SIMD_SIZE / type_size,
  E_SIMD_COUNT =
      (E_SIMD_COUNT_IM > 0
           ? E_SIMD_COUNT_IM
           : 1),  // Note that if a type is too big to fit, we pretend it can
  E_LOG_SIMD_COUNT = fetch::meta::Log2<E_SIMD_COUNT>::value,
  IS_SHARED = 1
};

static_assert(E_SIMD_COUNT == (1ull << E_LOG_SIMD_COUNT),
              "type does not fit in SIMD");

SharedArray(std::size_t const &n) {
  size_ = n;

  if (n > 0) {
    data_ = std::shared_ptr<T>(
        (type *)_mm_malloc(padded_size() * sizeof(type), 16), _mm_free);
  }
}

SharedArray() {}
SharedArray(SharedArray const &other)
    : size_(other.size_), data_(other.data_) {}

SharedArray(SharedArray &&other) {
  std::swap(size_, other.size_);
  std::swap(data_, other.data_);
}

SharedArray &operator=(SharedArray &&other) {
  std::swap(size_, other.size_);
  std::swap(data_, other.data_);
  return *this;
}

~SharedArray() {}

self_type Copy() const {
  // TODO: Use memcopy
  self_type ret(size_);
  for (std::size_t i = 0; i < size_; ++i) {
    ret[i] = At(i);
  }

  return ret;
}

ConstParallelDispatcher<type> in_parallel() const { return
ConstParallelDispatcher<type>(pointer(), size()); } ParallelDispatcher<type>
in_parallel() { return ParallelDispatcher<type>(pointer(), size()); }

void SetAllZero() { memset(data_.get(), 0, padded_size() * sizeof(type)); }

void SetPaddedZero() {
  memset(data_.get() + size(), 0, (padded_size() - size()) * sizeof(type));
}

void SetZeroAfter(std::size_t const &n) {
  memset(data_.get() + n, 0, (padded_size() - n) * sizeof(type));
}


iterator begin() { return iterator(data_.get(), data_.get() + size()); }
iterator end() {
  return iterator(data_.get() + size(), data_.get() + size());
}

iterator begin() const {
  return iterator(data_.get(), data_.get() + size());
}  // TODO: Implemnent const iterators
iterator end() const {
  return iterator(data_.get() + size(), data_.get() + size());
}

reverse_iterator rbegin() {
  return reverse_iterator(data_.get() + size() - 1, data_.get() - 1);
}
reverse_iterator rend() {
  return reverse_iterator(data_.get() - 1, data_.get() - 1);
}

T &operator[](std::size_t const &n) {
  assert(n < padded_size());
  return data_.get()[n];
}

T const &operator[](std::size_t const &n) const {
  assert(n < padded_size());
  return data_.get()[n];
}

T &At(std::size_t const &n) {
  assert(n < padded_size());
  return data_.get()[n];
}

T const &At(std::size_t const &n) const {
  assert(n < padded_size());
  return data_.get()[n];
}

T const &Set(std::size_t const &n, T const &v) {
  assert(n < padded_size());
  data_.get()[n] = v;
  return v;
}

self_type &operator=(SharedArray const &other) {
  if (&other == this) return *this;

  size_ = other.size_;
  data_ = other.data_;

  return *this;
}

std::size_t simd_size() const { return (size_) >> E_LOG_SIMD_COUNT; }

std::size_t size() const { return size_; }

std::size_t padded_size() const {
  std::size_t padded = std::size_t((size_) >> E_LOG_SIMD_COUNT)
                       << E_LOG_SIMD_COUNT;
  if (padded < size_) padded += E_SIMD_COUNT;
  return padded;
}

type *pointer() const { return data_.get(); }

private:

size_type size_ = 0;
data_type data_ = nullptr;
};
*/

}  // namespace memory
}  // namespace fetch
