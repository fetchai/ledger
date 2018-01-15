#ifndef MEMORY_SHARED_ARRAY_HPP
#define MEMORY_SHARED_ARRAY_HPP
#include <algorithm>
#include <atomic>
#include <cassert>
#include <cstdint>
#include <type_traits>
#include "iterator.hpp"
#include "meta/log2.hpp"
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
  typedef std::atomic<uint64_t> counter_type;
  typedef ForwardIterator<T> iterator;
  typedef BackwardIterator<T> reverse_iterator;
  typedef SharedArray<T> self_type;
  typedef T type;

  enum {
    E_SIMD_SIZE = 16,
    E_SIMD_COUNT = E_SIMD_SIZE / sizeof(T),
    E_LOG_SIMD_COUNT = details::meta::Log2<E_SIMD_COUNT>::value
  };

  static_assert(E_SIMD_COUNT == (1ull << E_LOG_SIMD_COUNT),
                "type does not fit in SIMD");

  SharedArray(std::size_t const &n) {
    ref_count_ = new counter_type(1);
    ;
    size_ = new std::size_t(n);

    if (n > 0) data_ = new type[padded_size()];

#ifdef FETCH_TESTING_ENABLED
    ++testing::total_shared_objects;
#endif
  }

  SharedArray() : SharedArray(0) {}

  SharedArray(SharedArray const &other) { Assign(other); }

  SharedArray(SharedArray &&other) {
    std::swap(size_, other.size_);
    std::swap(ref_count_, other.ref_count_);
    std::swap(data_, other.data_);
  }

  SharedArray &operator=(SharedArray &&other) {
    std::swap(size_, other.size_);
    std::swap(ref_count_, other.ref_count_);
    std::swap(data_, other.data_);
    return *this;
  }

  ~SharedArray() { Decrease(); }

  iterator begin() { return iterator(data_, data_ + size()); }
  iterator end() { return iterator(data_ + size(), data_ + size()); }
  reverse_iterator rbegin() {
    return reverse_iterator(data_ + size() - 1, data_ - 1);
  }
  reverse_iterator rend() { return reverse_iterator(data_ - 1, data_ - 1); }

  T &operator[](std::size_t const &n) {
    assert(data_ != nullptr);
    assert(n < size());
    return data_[n];
  }

  T const &operator[](std::size_t const &n) const {
    assert(data_ != nullptr);
    assert(n < size());
    return data_[n];
  }

  T &At(std::size_t const &n) {
    assert(data_ != nullptr);
    assert(n < size());
    return data_[n];
  }

  T const &At(std::size_t const &n) const {
    assert(data_ != nullptr);
    assert(n < size());
    return data_[n];
  }

  T const &Set(std::size_t const &n, T const &v) {
    assert(data_ != nullptr);
    assert(n < size());
    data_[n] = v;
    return v;
  }

  self_type &operator=(SharedArray const &other) {
    if (&other == this) return *this;

    Decrease();
    Assign(other);

    return *this;
  }

  uint64_t reference_count() const {
    assert(ref_count_ != nullptr);
    return *ref_count_;
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

  type *pointer() const { return data_; }

 private:
  void Decrease() {
    assert(ref_count_ != nullptr);
    --(*ref_count_);
    if ((*ref_count_) == 0) {
      if (size_) delete size_;
      if (data_) delete[] data_;

      if (ref_count_) delete ref_count_;
#ifdef FETCH_TESTING_ENABLED
      --testing::total_shared_objects;
#endif
    }
  }
  void Assign(self_type const &other) {
    size_ = other.size_;
    ref_count_ = other.ref_count_;
    data_ = other.data_;
    ++(*ref_count_);
  }

  std::size_t *size_ = nullptr;
  counter_type *ref_count_ = nullptr;
  T *data_ = nullptr;
};
};
};
#endif
