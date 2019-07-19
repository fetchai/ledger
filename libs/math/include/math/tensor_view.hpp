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

#include <cassert>
#include <type_traits>
#include <utility>

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

  IteratorType      begin();
  IteratorType      end();
  ConstIteratorType cbegin() const;
  ConstIteratorType cend() const;

  void         Assign(TensorView const &other);
  void         Assign(Tensor<T, C> const &other);
  Tensor<T, C> Copy() const;

  /////////////////
  /// OPERATORS ///
  /////////////////

  template <typename S>
  typename std::enable_if<std::is_integral<S>::value, Type>::type operator()(S i, S j) const;
  template <typename S>
  typename std::enable_if<std::is_integral<S>::value, Type>::type &operator()(S i, S j);
  template <typename S>
  typename std::enable_if<std::is_integral<S>::value, Type>::type operator()(S i) const;
  template <typename S>
  typename std::enable_if<std::is_integral<S>::value, Type>::type &operator()(S i);
  template <typename S>
  typename std::enable_if<std::is_integral<S>::value, Type>::type operator[](S i) const;
  template <typename S>
  typename std::enable_if<std::is_integral<S>::value, Type>::type &operator[](S i);

  /////////////
  /// SIZES ///
  /////////////

  static SizeType PadValue(SizeType size);

  SizeType           height() const;
  SizeType           width() const;
  SizeType           padded_size() const;
  SizeType           padded_height() const;
  constexpr SizeType padding();

  //////////////////
  /// ACCCESSORS ///
  //////////////////

  ContainerType const &data() const;
  ContainerType &      data();

private:
  SizeType      height_{0};
  SizeType      width_{0};
  SizeType      padded_height_{0};
  ContainerType data_{};
};

//////////////////////
/// PUBLIC METHODS ///
//////////////////////

/**
 * Returns an iterator over the relevant view of the tensor starting at the beginning
 * @tparam T
 * @tparam C
 * @return
 */
template <typename T, typename C>
typename TensorView<T, C>::IteratorType TensorView<T, C>::begin()
{
  SizeType padded_size = padded_height_ * width_;
  return IteratorType(data_.pointer(), height_ * width_, padded_size, height_, padded_height_);
}

/**
 * Returns an iterator over the relevant view of the tensor starting at the end
 * @tparam T
 * @tparam C
 * @return
 */
template <typename T, typename C>
typename TensorView<T, C>::IteratorType TensorView<T, C>::end()
{
  SizeType padded_size = padded_height_ * width_;
  return IteratorType(data_.pointer() + padded_size, height_ * width_, padded_size, height_,
                      padded_height_);
}

/**
 * Returns an iterator over the relevant view of the constant tensor starting at the beginning
 * @tparam T
 * @tparam C
 * @return
 */
template <typename T, typename C>
typename TensorView<T, C>::ConstIteratorType TensorView<T, C>::cbegin() const
{
  return ConstIteratorType(data_.pointer(), height_ * width_, padded_height_ * width_, height_,
                           padded_height_);
}

/**
 * Returns an iterator over the relevant view of the constant tensor starting at the end
 * @tparam T
 * @tparam C
 * @return
 */
template <typename T, typename C>
typename TensorView<T, C>::ConstIteratorType TensorView<T, C>::cend() const
{
  SizeType padded_size = padded_height_ * width_;
  return ConstIteratorType(data_.pointer() + padded_size, height_ * width_, padded_size, height_,
                           padded_height_);
}

/**
 * Assigns the contents of one tensorview to another
 * @tparam T
 * @tparam C
 * @param other
 */
template <typename T, typename C>
void TensorView<T, C>::Assign(TensorView const &other)
{
  auto it1 = begin();
  auto it2 = other.cbegin();
  assert(it1.size() == it2.size());
  while (it1.is_valid())
  {
    *it1 = *it2;
    ++it1;
    ++it2;
  }
}

/**
 * Assigns the entire contents of one tensor to this tensorview
 * @tparam T
 * @tparam C
 * @param other
 */
template <typename T, typename C>
void TensorView<T, C>::Assign(Tensor<T, C> const &other)
{
  auto other_view = other.View();
  Assign(other_view);
}

/**
 * Construct a new Tensor by copying the data specified by the view
 * @tparam T
 * @tparam C
 * @return
 */
template <typename T, typename C>
Tensor<T, C> TensorView<T, C>::Copy() const
{
  Tensor<T, C> ret({height_, width_});
  ret.Assign(*this);
  return ret;
}

template <typename T, typename C>
template <typename S>
typename std::enable_if<std::is_integral<S>::value, T>::type TensorView<T, C>::operator()(S i,
                                                                                          S j) const
{
  return data_[static_cast<SizeType>(i) + static_cast<SizeType>(j) * padded_height_];
}

template <typename T, typename C>
template <typename S>
typename std::enable_if<std::is_integral<S>::value, T>::type &TensorView<T, C>::operator()(S i, S j)
{
  return data_[static_cast<SizeType>(i) + static_cast<SizeType>(j) * padded_height_];
}

template <typename T, typename C>
template <typename S>
typename std::enable_if<std::is_integral<S>::value, T>::type TensorView<T, C>::operator()(S i) const
{
  return data_[std::move(i)];
}

template <typename T, typename C>
template <typename S>
typename std::enable_if<std::is_integral<S>::value, T>::type &TensorView<T, C>::operator()(S i)
{
  return data_[std::move(i)];
}

template <typename T, typename C>
template <typename S>
typename std::enable_if<std::is_integral<S>::value, T>::type TensorView<T, C>::operator[](S i) const
{
  return data_[std::move(i)];
}

template <typename T, typename C>
template <typename S>
typename std::enable_if<std::is_integral<S>::value, T>::type &TensorView<T, C>::operator[](S i)
{
  return data_[std::move(i)];
}

/**
 * @brief returns the smallest number which is a multiple of PADDING and greater than or equal to a
 * desired size
 * @param sizeis the size to be padded
 * @return the padded size
 */
template <typename T, typename C>
SizeType TensorView<T, C>::PadValue(SizeType size)
{
  SizeType ret = SizeType(size / PADDING) * PADDING;
  if (ret < size)
  {
    ret += PADDING;
  }
  return ret;
}

template <typename T, typename C>
SizeType TensorView<T, C>::height() const
{
  return height_;
}

template <typename T, typename C>
SizeType TensorView<T, C>::width() const
{
  return width_;
}

template <typename T, typename C>
SizeType TensorView<T, C>::padded_size() const
{
  return data_.padded_size();
}

template <typename T, typename C>
SizeType TensorView<T, C>::padded_height() const
{
  return padded_height_;
}

template <typename T, typename C>
constexpr SizeType TensorView<T, C>::padding()
{
  return PADDING;
}

template <typename T, typename C>
typename TensorView<T, C>::ContainerType const &TensorView<T, C>::data() const
{
  return data_;
}

template <typename T, typename C>
typename TensorView<T, C>::ContainerType &TensorView<T, C>::data()
{
  return data_;
}

}  // namespace math
}  // namespace fetch
