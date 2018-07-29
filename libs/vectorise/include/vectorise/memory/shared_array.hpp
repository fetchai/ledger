#pragma once

#include "vectorise/memory/iterator.hpp"
#include "vectorise/memory/parallel_dispatcher.hpp"
#include "vectorise/memory/vector_slice.hpp"
#include "vectorise/meta/log2.hpp"

#include <algorithm>
#include <atomic>
#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <mm_malloc.h>
#include <type_traits>
namespace fetch {
namespace memory {

template <typename T, std::size_t type_size = sizeof(T)>
class SharedArray : public VectorSlice<T, type_size>
{
public:
  //static_assert(sizeof(T) >= type_size, "The size allocated to the type must at least be larger than the type itself");
//  static_assert(std::is_pod<T>::value, "The shared array can only be used with plain old data types");

  using size_type = std::size_t;
  using data_type = std::shared_ptr<T>;
  using super_type = VectorSlice<T, type_size>;
  using self_type = SharedArray<T, type_size>;
  using type      = T;

  SharedArray(std::size_t const &n)
    : super_type()
  {
    this->size_ = n;

    if (n > 0)
    {
      data_ = std::shared_ptr<T>(
        reinterpret_cast<type *>(
          _mm_malloc(this->padded_size() * sizeof(type), 16)
        ),
        _mm_free
      );

      this->pointer_ = data_.get();

      Initialise();
    }
  }

  SharedArray() = default;
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

    if (other.data_)
    {
      this->data_ = other.data_;
    }
    else
    {
      this->data_.reset();
    }

    this->pointer_ = other.pointer_;
    return *this;
  }

  ~SharedArray()
  {
    CleanUp();
  }

  self_type Copy() const
  {
    // TODO(unknown): Use memcopy
    self_type ret(this->size_);
    for (std::size_t i = 0; i < this->size_; ++i)
    {
      ret[i] = this->At(i);
    }

    return ret;
  }

private:

  template <typename R = T>
  typename std::enable_if<std::is_pod<R>::value>::type
  Initialise()
  {
    this->SetAllZero();
  }

  template <typename R = T>
  typename std::enable_if<(!std::is_pod<R>::value) && std::is_default_constructible<R>::value>::type
  Initialise()
  {
    for (std::size_t i = 0; i < this->size_; ++i)
    {
      // construct the object in place
      new (&(*this)[i]) T();
    }
  }

  template <typename R = T>
  typename std::enable_if<std::is_pod<R>::value>::type
  CleanUp()
  {
    this->SetAllZero();
  }

  template <typename R = T>
  typename std::enable_if<(!std::is_pod<R>::value) && std::is_default_constructible<R>::value>::type
  CleanUp()
  {
    if (data_)
    {
      for (std::size_t i = 0; i < this->size_; ++i)
      {
        // destruct the object in place
        (*this)[i].~T();
      }
    }
  }

  data_type data_ = nullptr;
};

/*
template <typename T, std::size_t type_size = sizeof(T)>
class SharedArray {
public:
using size_type = std::size_t;
using data_type = std::shared_ptr<T>;

using iterator = ForwardIterator<T>;
using reverse_iterator = BackwardIterator<T>;
using self_type = SharedArray<T, type_size>;
using type = T;

using const_parallel_dispatcher_type = ConstParallelDispatcher<type>;
using parallel_dispatcher_type = ParallelDispatcher<type>;
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
