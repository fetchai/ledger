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

namespace fetch {
namespace math {

template <typename T, typename C>
class Tensor;

template <typename T, typename C, typename TensorType = Tensor<T, C>>
class TensorIterator
{
public:
  using Type     = T;
  using SizeType = uint64_t;
  /**
   * default range assumes step 1 over whole array - useful for trivial cases
   * @param array
   */
  TensorIterator(TensorType &array)
    : array_(array)
  {}

  TensorIterator(TensorIterator const &other) = default;
  TensorIterator &operator=(TensorIterator const &other) = default;
  TensorIterator(TensorIterator &&other)                 = default;
  TensorIterator &operator=(TensorIterator &&other) = default;

  TensorIterator(TensorType &array, SizeType position)
    : array_(array)
    , position_{std::move(position)}
  {}

  static TensorIterator EndIterator(TensorType &array)
  {
    return TensorIterator(array, array.data().size());
  }
  /**
   * identifies whether the iterator is still valid or has finished iterating
   * @return boolean indicating validity
   */
  bool is_valid() const
  {
    return position_ < array_.data().size();
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
    ++position_;

    if (i_ >= array_.height())
    {
      i_ = 0;
      ++j_;
      position_ = j_ * array_.padded_height();
    }

    return *this;
  }

  /**
   * dereference, i.e. give the value at the current position of the iterator
   * @return
   */
  Type &operator*()
  {
    assert(position_ < array_.data().size());
    return array_.data()[position_];
  }

  Type const &operator*() const
  {
    assert(position_ < array_.data().size());
    return array_.data()[position_];
  }

  bool operator==(TensorIterator const &other) const
  {
    return other.position_ == position_;
  }

  bool operator!=(TensorIterator const &other) const
  {
    return other.position_ != position_;
  }

  SizeType size() const
  {
    return array_.size();
  }

private:
  TensorType &array_;
  SizeType    position_{0};
  SizeType    i_{0};
  SizeType    j_{0};
};

template <typename T, typename C>
using ConstTensorIterator = TensorIterator<T const, C, Tensor<T, C> const>;

}  // namespace math
}  // namespace fetch
