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

#include "math/base_types.hpp"

#include <cassert>

namespace fetch {
namespace math {

template <typename T>
class TensorIterator
{
public:
  using Type = T;
  /**
   * default range assumes step 1 over whole array - useful for trivial cases
   * @param array
   */
  TensorIterator(T *pointer, SizeType size, SizeType padded_size, SizeType height,
                 SizeType padded_height)
  {
    pointer_ = pointer;
    skip_    = padded_height - height;
    end_     = pointer_ + padded_size;
    height_  = height;
    size_    = size;
  }

  TensorIterator(TensorIterator const &other) = default;
  TensorIterator &operator=(TensorIterator const &other) = default;
  TensorIterator(TensorIterator &&other)                 = default;
  TensorIterator &operator=(TensorIterator &&other) = default;

  /**
   * identifies whether the iterator is still valid or has finished iterating
   * @return boolean indicating validity
   */
  bool is_valid() const
  {
    return pointer_ < end_;
  }

  /**
   * same as is_valid
   * @return
   */
  operator bool() const
  {
    return is_valid();
  }

  /**
   * incrementer, i.e. increment through the memory by 1 position making n-dim adjustments as
   * necessary
   * @return
   */
  TensorIterator &operator++()
  {
    ++i_;
    ++pointer_;

    if (i_ >= height_)
    {
      i_ = 0;
      ++j_;
      pointer_ += skip_;
    }

    return *this;
  }

  /**
   * dereference, i.e. give the value at the current position of the iterator
   * @return
   */
  Type &operator*()
  {
    assert(is_valid());
    return *pointer_;
  }

  Type const &operator*() const
  {
    assert(is_valid());
    return *pointer_;
  }

  bool operator==(TensorIterator const &other) const
  {
    return other.pointer_ == pointer_;
  }

  bool operator!=(TensorIterator const &other) const
  {
    return other.pointer_ != pointer_;
  }

  SizeType size() const
  {
    return size_;
  }

private:
  T *pointer_;
  T *end_;

  SizeType height_{0};
  SizeType skip_{0};
  SizeType i_{0};
  SizeType j_{0};
  SizeType size_{0};
};

template <typename T>
using ConstTensorIterator = TensorIterator<T const>;

}  // namespace math
}  // namespace fetch
