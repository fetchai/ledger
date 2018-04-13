#ifndef MEMORY_SHARED_ARRAY_HPP
#define MEMORY_SHARED_ARRAY_HPP
#include "iterator.hpp"
#include "meta/log2.hpp"


#include <algorithm>
#include <atomic>
#include <cassert>
#include <cstdint>
#include<cstring>
#include <type_traits>
#include <memory>
#include <iostream>
#include <stdlib.h>
#include <mm_malloc.h>
namespace fetch {
namespace memory {

#ifdef FETCH_TESTING_ENABLED
namespace testing {
std::size_t total_shared_objects = 0;
};
#endif

template <typename T, std::size_t type_size = sizeof(T)>
class SharedArray {
 public:
  typedef std::size_t size_type;
  typedef std::shared_ptr<T> data_type;

  typedef ForwardIterator<T> iterator;
  typedef BackwardIterator<T> reverse_iterator;
  typedef SharedArray<T, type_size> self_type;
  typedef T type;

  
  enum {
    E_SIMD_SIZE = 16,
    E_SIMD_COUNT_IM = E_SIMD_SIZE / type_size,
    E_SIMD_COUNT = (E_SIMD_COUNT_IM > 0 ? E_SIMD_COUNT_IM : 1 ), // Note that if a type is too big to fit, we pretend it can
    E_LOG_SIMD_COUNT = fetch::meta::Log2<E_SIMD_COUNT>::value
  };

  static_assert(E_SIMD_COUNT == (1ull << E_LOG_SIMD_COUNT),
                "type does not fit in SIMD");
  
  SharedArray(std::size_t const &n) {
    size_ = n;

    if (n > 0)
    {
      data_ = std::shared_ptr<T>( (type*)_mm_malloc(padded_size()*sizeof(type), 16 ), _mm_free );
    }
  }
  
  SharedArray() {}
  SharedArray(SharedArray const &other)
    :size_(other.size_), data_(other.data_) {
  }

  SharedArray(SharedArray &&other) {
    std::swap(size_, other.size_);
    std::swap(data_, other.data_);
  }

  SharedArray &operator=(SharedArray &&other) {
    std::swap(size_, other.size_);
    std::swap(data_, other.data_);
    return *this;
  }

  ~SharedArray() { }

  self_type Copy() const
  {
    self_type ret( size_ );
    for(std::size_t i = 0; i < size_; ++i )
    {
      ret[i] = At( i );
    }
    
    return ret;
  }
  
  void SetAllZero() {
    memset( data_.get(), 0, padded_size()*sizeof(type) );
  }
  
  iterator begin() { return iterator(data_.get(), data_.get() + size()); }
  iterator end() { return iterator(data_.get() + size(), data_.get() + size()); }

  iterator begin() const { return iterator(data_.get(), data_.get() + size()); } // TODO: Implemnent const iterators
  iterator end() const { return iterator(data_.get() + size(), data_.get() + size()); }
  
  reverse_iterator rbegin() {
    return reverse_iterator(data_.get() + size() - 1, data_.get() - 1);
  }
  reverse_iterator rend() { return reverse_iterator(data_.get() - 1, data_.get() - 1); }

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


  std::size_t simd_size() const {
    return (size_) >> E_LOG_SIMD_COUNT;
  }

  std::size_t size() const {
    return size_;
  }

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
};
};
#endif
