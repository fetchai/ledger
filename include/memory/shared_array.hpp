#ifndef MEMORY_SHARED_ARRAY_HPP
#define MEMORY_SHARED_ARRAY_HPP
#include "iterator.hpp"
#include "meta/log2.hpp"


#include <algorithm>
#include <atomic>
#include <cassert>
#include <cstdint>
#include <type_traits>
#include <memory>
#include <iostream>
#include <stdlib.h>
#include <string.h>
namespace fetch {
namespace memory {

#ifdef FETCH_TESTING_ENABLED
namespace testing {
std::size_t total_shared_objects = 0;
};
#endif

template <typename T>
class SharedArray {
 public:
  typedef std::shared_ptr< std::size_t > size_type;
  typedef std::shared_ptr<T> data_type;

  typedef ForwardIterator<T> iterator;
  typedef BackwardIterator<T> reverse_iterator;
  typedef SharedArray<T> self_type;
  typedef T type;

  
  enum {
    E_SIMD_SIZE = 16,
    E_SIMD_COUNT_IM = E_SIMD_SIZE / sizeof(T),
    E_SIMD_COUNT = (E_SIMD_COUNT_IM > 0 ? E_SIMD_COUNT_IM : 1 ), // Note that if a type is too big to fit, we pretend it can
    E_LOG_SIMD_COUNT = details::meta::Log2<E_SIMD_COUNT>::value
  };

  static_assert(E_SIMD_COUNT == (1ull << E_LOG_SIMD_COUNT),
                "type does not fit in SIMD");
  
  SharedArray(std::size_t const &n) {   
    size_ = std::shared_ptr< std::size_t  >( new std::size_t (n) );

    if (n > 0)
    {
      // TODO: C++17 version, upgrade when time is right
      //      data_ = std::shared_ptr<T>( (type*)std::aligned_alloc(E_SIMD_SIZE, padded_size()*sizeof(type) ), free );

      // TODO: Upgrade to aligned memory to ensure that we can parallelise over SIMD
      data_ = std::shared_ptr<T>( (type*)malloc(padded_size()*sizeof(type) ), free );
      memset( data_.get(), 0, padded_size()*sizeof(type) );
    }
  }
  
  SharedArray() : SharedArray(0) {}
  SharedArray(SharedArray const &other)
    :size_(other.size_), data_(other.data_) { }

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

  SharedArray< type > Copy() const
  {
    SharedArray< type > ret( *size_ );
    for(std::size_t i = 0; i < (*size_); ++i )
    {
      ret[i] = At( i );
    }
    
    return ret;
  }
  
  
  iterator begin() { return iterator(data_.get(), data_.get() + size()); }
  iterator end() { return iterator(data_.get() + size(), data_.get() + size()); }
  reverse_iterator rbegin() {
    return reverse_iterator(data_.get() + size() - 1, data_.get() - 1);
  }
  reverse_iterator rend() { return reverse_iterator(data_.get() - 1, data_.get() - 1); }

  T &operator[](std::size_t const &n) {
    assert(n < size());
    return data_.get()[n];
  }

  T const &operator[](std::size_t const &n) const {
    assert(n < size());
    return data_.get()[n];
  }

  T &At(std::size_t const &n) {
    assert(n < size());
    return data_.get()[n];
  }

  T const &At(std::size_t const &n) const {
    assert(n < size());
    return data_.get()[n];
  }

  T const &Set(std::size_t const &n, T const &v) {
    assert(n < size());
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
    assert(size_ != nullptr);
    return (*size_) >> E_LOG_SIMD_COUNT;
  }

  std::size_t size() const {
    assert(size_ != nullptr);
    return *size_;
  }

  std::size_t padded_size() const {
    assert(size_ != nullptr);
    std::size_t padded = std::size_t((*size_) >> E_LOG_SIMD_COUNT)
                         << E_LOG_SIMD_COUNT;
    if (padded < *size_) padded += E_SIMD_COUNT;
    return padded;
  }

  type *pointer() const { return data_.get(); }

 private:
  size_type size_;
  data_type data_;
};
};
};
#endif
