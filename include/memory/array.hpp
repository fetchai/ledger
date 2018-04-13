#ifndef MEMORY_ARRAY_HPP
#define MEMORY_ARRAY_HPP
#include <algorithm>
#include <atomic>
#include <cassert>
#include <cstdint>
#include <type_traits>
#include "iterator.hpp"
#include "meta/log2.hpp"
namespace fetch {
namespace memory {

template <typename T>
class Array {
 public:
  typedef std::size_t size_type;
  typedef T* data_type;
  
  typedef ForwardIterator<T> iterator;
  typedef BackwardIterator<T> reverse_iterator;
  typedef Array<T> self_type;
  typedef T type;

  enum {
    E_SIMD_SIZE = 16,
    E_SIMD_COUNT = E_SIMD_SIZE / sizeof(T),
    E_LOG_SIMD_COUNT = fetch::meta::Log2<E_SIMD_COUNT>::value
  };

  static_assert(E_SIMD_COUNT == (1ull << E_LOG_SIMD_COUNT),
                "type does not fit in SIMD");

  Array(std::size_t const &n) {
    size_ = n;
    if (n > 0) data_ = new type[padded_size()];
  }

  Array() : Array(0) {}

  Array(Array const &other)
  {
    this->operator=(other);
  }

  Array(Array &&other) {
    std::swap(size_, other.size_);
    std::swap(data_, other.data_);
  }

  
  Array &operator=(Array &&other) {
    std::swap(size_, other.size_);
    std::swap(data_, other.data_);
    return *this;
  }

  ~Array() {
    if(data_ != nullptr) delete[] data_;
  }

  Array< type > Copy() const
  {
    Array< type > ret( size_ );
    for(std::size_t i = 0; i < size_; ++i )
    {
      ret[i] = At( i );
    }
    
    return ret;
  }
  

  
  iterator begin() { return iterator(data_, data_ + size()); }
  iterator end() { return iterator(data_ + size(), data_ + size()); }
  reverse_iterator rbegin() {
    return reverse_iterator(data_ + size() - 1, data_ - 1);
  }
  reverse_iterator rend() { return reverse_iterator(data_ - 1, data_ - 1); }

  T &operator[](std::size_t const &n) {
    assert(data_ != nullptr);
    assert(n < padded_size());
    return data_[n];
  }

  T const &operator[](std::size_t const &n) const {
    assert(data_ != nullptr);
    
    assert(n < padded_size());
    return data_[n];
  }

  T &At(std::size_t const &n) {
    assert(data_ != nullptr);
    assert(n < padded_size());
    return data_[n];
  }

  T const &At(std::size_t const &n) const {
    assert(data_ != nullptr);
    assert(n < padded_size());
    return data_[n];
  }

  T const &Set(std::size_t const &n, T const &v) {
    assert(data_ != nullptr);
    assert(n < padded_size());
    data_[n] = v;
    return v;
  }

  self_type &operator=(Array const &other) {
    if (&other == this) return *this;

    if(data_ != nullptr) delete[] data_;
    data_ = new T[other.padded_size()];
    size_ = other.size();
    
    for(std::size_t i=0; i < size_; ++i)
      data_[i] = other[i];
    
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

  type *pointer() const { return data_; }

 private:

  size_type size_ = 0;
  T *data_ = nullptr;
};
};
};
#endif
