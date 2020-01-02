#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018-2020 Fetch.AI Limited
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
#include "vectorise/fixed_point/type_traits.hpp"
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
  static_assert(fetch::math::meta::IsPodOrFixedPoint<T>, "can only be used with POD or FixedPoint");
  using SizeType  = std::size_t;
  using DataType  = std::shared_ptr<T>;
  using SuperType = VectorSlice<T, type_size>;
  using SelfType  = Array<T, type_size>;
  using type      = T;

  explicit Array(std::size_t n)
  {
    this->size_ = n;

    if (n > 0)
    {
      this->pointer_ = reinterpret_cast<type *>(_mm_malloc(this->padded_size() * sizeof(type), 32));
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

  Array(Array &&other) noexcept
  {
    std::swap(this->size_, other.size_);
    std::swap(this->pointer_, other.pointer_);
  }

  Array &operator=(Array &&other) noexcept
  {
    std::swap(this->size_, other.size_);
    std::swap(this->pointer_, other.pointer_);

    return *this;
  }

  Array(Array const &other)
    : SuperType()
  {
    this->operator=(other);
  }

  SelfType &operator=(Array const &other)  // NOLINT
  {
    if (this->pointer_ != nullptr)
    {
      _mm_free(this->pointer_);
    }
    this->size_ = other.size();

    if (this->size_ > 0)
    {
      this->pointer_ = reinterpret_cast<type *>(_mm_malloc(this->padded_size() * sizeof(type), 32));
    }

    for (std::size_t i = 0; i < this->size_; ++i)
    {
      this->pointer_[i] = other.At(i);
    }
    return *this;
  }

  SelfType Copy() const
  {
    SelfType ret = *this;
    return ret;
  }
};
}  // namespace memory
}  // namespace fetch
