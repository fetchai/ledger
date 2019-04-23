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

#include "tensor.hpp"

namespace fetch {
namespace math {
template <typename T, typename SizeType>
class TensorIterator
{
  using Type = T;

  friend class Tensor<T>;

private:
  TensorIterator(std::vector<SizeType> const &shape, std::vector<SizeType> const &strides,
                 std::vector<SizeType> const &padding, std::vector<SizeType> const &coordinate,
                 std::shared_ptr<std::vector<T>> const &storage, SizeType const &offset)
    : shape_(shape)
    , strides_(strides)
    , padding_(padding)
    , coordinate_(coordinate)
  {
    pointer_          = storage->data() + offset;
    original_pointer_ = storage->data() + offset;
  }

public:
  bool operator!=(const TensorIterator<T, SizeType> &other) const
  {
    return !(*this == other);
  }

  bool operator==(const TensorIterator<T, SizeType> &other) const
  {
    return (original_pointer_ == other.original_pointer_) && (coordinate_ == other.coordinate_);
  }

  TensorIterator &operator++()
  {
    pointer_ += strides_.back();
    coordinate_.back()++;
    if (coordinate_.back() < shape_.back())
    {
      return *this;
    }
    std::uint64_t i{shape_.size() - 1};
    while (i && (coordinate_[i] >= shape_[i]))
    {
      coordinate_[i] = 0;
      coordinate_[(i ? i - 1 : i)] += 1;
      i--;
    }
    pointer_ = original_pointer_;
    for (std::size_t i(0); i < coordinate_.size(); ++i)
    {
      pointer_ += coordinate_[i] * strides_[i];
    }
    return *this;
  }

  T &operator*() const
  {
    return *pointer_;
  }

private:
  const std::vector<SizeType> shape_;
  const std::vector<SizeType> strides_;
  const std::vector<SizeType> padding_;
  std::vector<SizeType>       coordinate_;
  T *                         pointer_;
  T *                         original_pointer_;
};
}  // namespace math
}  // namespace fetch
