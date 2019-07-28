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
#include "vectorise/memory/vector_slice.hpp"

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
class Array : public VectorSlice<T, type_size>
{
public:
  static_assert(sizeof(T) >= type_size, "Invalid object size");
  // TODO(issue 1424): check IfIsPodOrFixedPoint memory safe and reinstante appropriate static asserts
  //  static_assert(std::is_pod<T>::value, "Can only be used with POD types");
  //  static_assert(meta::IfIsPodOrFixedPoint<T>::value, "can only be used with POD or FixedPoint");
  using SizeType   = std::size_t;
  using data_type  = std::shared_ptr<T>;
  using super_type = VectorSlice<T, type_size>;
  using self_type  = Array<T, type_size>;
  using type       = T;

  Array(std::size_t n)
  {
    this->size_ = n;

    if (n > 0)
    {
      this->pointer_ = (type *)_mm_malloc(this->padded_size() * sizeof(type), 16);
    }
  }

  ~Array()
  {
    if (this->pointer_ != nullptr)
    {
      _mm_free(this->pointer_);
    }
  }

  Array() = default;

  Array(Array &&other)
  {
    std::swap(this->size_, other.size_);
    std::swap(this->pointer_, other.pointer_);
  }

  Array &operator=(Array &&other)
  {
    std::swap(this->size_, other.size_);

    std::swap(this->pointer_, other.pointer_);
    return *this;
  }

  Array(Array const &other)
    : super_type()
  {
    this->operator=(other);
  }

  self_type &operator=(Array const &other)
  {
    if (this->pointer_ != nullptr)
    {
      _mm_free(this->pointer_);
    }
    this->size_ = other.size();

    if (this->size_ > 0)
    {
      this->pointer_ = (type *)_mm_malloc(this->padded_size() * sizeof(type), 16);
    }

    for (std::size_t i = 0; i < this->size_; ++i)
    {
      this->pointer_[i] = other.At(i);
    }
    return *this;
  }

  self_type Copy() const
  {
    self_type ret = *this;
    return ret;
  }
};
}  // namespace memory
}  // namespace fetch
