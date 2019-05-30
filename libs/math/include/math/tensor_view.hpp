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
#include "math/tensor_iterator.hpp"

namespace fetch {
namespace math {
template <typename T, typename C = memory::SharedArray<T>>
class TensorView
{
public:
  using Type                       = T;
  using ContainerType              = C;
  using IteratorType               = TensorIterator<T>;
  using ConstIteratorType          = ConstTensorIterator<T>;
  using VectorSliceType            = typename ContainerType::VectorSliceType;
  using VectorRegisterType         = typename ContainerType::VectorRegisterType;
  using VectorRegisterIteratorType = typename ContainerType::VectorRegisterIteratorType;

  enum
  {
    LOG_PADDING = 2,
    PADDING     = static_cast<SizeType>(1) << LOG_PADDING
  };

  TensorView(ContainerType data, SizeType height, SizeType width, SizeType offset = 0)
    : height_{std::move(height)}
    , width_{std::move(width)}
    , padded_height_{PadValue(height_)}
    , data_{std::move(data), offset, padded_height_ * width_}
  {}

  IteratorType begin()
  {
    SizeType padded_size = padded_height_ * width_;
    return IteratorType(data_.pointer(), height_ * width_, padded_size, height_, padded_height_);
  }

  IteratorType end()
  {
    SizeType padded_size = padded_height_ * width_;
    return IteratorType(data_.pointer() + padded_size, height_ * width_, padded_size, height_,
                        padded_height_);
  }

  ConstIteratorType begin() const
  {
    return ConstIteratorType(data_.pointer(), height_ * width_, padded_height_ * width_, height_,
                             padded_height_);
  }

  ConstIteratorType end() const
  {
    SizeType padded_size = padded_height_ * width_;
    return ConstIteratorType(data_.pointer() + padded_size, height_ * width_, padded_size, height_,
                             padded_height_);
  }

  ConstIteratorType cbegin() const
  {
    return ConstIteratorType(data_.pointer(), height_ * width_, padded_height_ * width_, height_,
                             padded_height_);
  }

  ConstIteratorType cend() const
  {
    SizeType padded_size = padded_height_ * width_;
    return ConstIteratorType(data_.pointer() + padded_size, height_ * width_, padded_size, height_,
                             padded_height_);
  }

  template <typename S>
  typename std::enable_if<std::is_integral<S>::value, Type>::type operator()(S i, S j) const
  {
    return data_[static_cast<SizeType>(i) + static_cast<SizeType>(j) * padded_height_];
  }

  template <typename S>
  typename std::enable_if<std::is_integral<S>::value, Type>::type &operator()(S i, S j)
  {
    return data_[static_cast<SizeType>(i) + static_cast<SizeType>(j) * padded_height_];
  }

  template <typename S>
  typename std::enable_if<std::is_integral<S>::value, Type>::type operator()(S i) const
  {
    return data_[std::move(i)];
  }

  template <typename S>
  typename std::enable_if<std::is_integral<S>::value, Type>::type &operator()(S i)
  {
    return data_[std::move(i)];
  }

  template <typename S>
  typename std::enable_if<std::is_integral<S>::value, Type>::type operator[](S i) const
  {
    return data_[std::move(i)];
  }

  template <typename S>
  typename std::enable_if<std::is_integral<S>::value, Type>::type &operator[](S i)
  {
    return data_[std::move(i)];
  }

  /* @breif returns the smallest number which is a multiple of PADDING and greater than or equal to
   a desired size.
   * @param size is the size to be padded.
   & @returns the padded size
   */
  static SizeType PadValue(SizeType size)
  {
    SizeType ret = SizeType(size / PADDING) * PADDING;
    if (ret < size)
    {
      ret += PADDING;
    }
    return ret;
  }

  SizeType height() const
  {
    return height_;
  }

  SizeType width() const
  {
    return width_;
  }

  SizeType padded_size() const
  {
    return data_.padded_size();
  }

  SizeType padded_height() const
  {
    return padded_height_;
  }

  constexpr SizeType padding()
  {
    return PADDING;
  }

  ContainerType const &data() const
  {
    return data_;
  }

  ContainerType &data()
  {
    return data_;
  }

private:
  SizeType      height_{0};
  SizeType      width_{0};
  SizeType      padded_height_{0};
  ContainerType data_{};
};

}  // namespace math
}  // namespace fetch