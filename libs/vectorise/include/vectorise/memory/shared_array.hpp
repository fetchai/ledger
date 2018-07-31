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


}  // namespace memory
}  // namespace fetch
