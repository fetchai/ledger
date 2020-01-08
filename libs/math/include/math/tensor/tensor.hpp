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

#include "core/macros.hpp"
#include "core/random.hpp"
#include "core/serializers/group_definitions.hpp"
#include "math/exceptions/exceptions.hpp"
#include "math/fundamental_operators.hpp"
#include "math/matrix_operations.hpp"
#include "math/tensor/tensor_iterator.hpp"
#include "math/tensor/tensor_slice_iterator.hpp"
#include "math/tensor/tensor_view.hpp"

#include <cassert>
#include <cstdint>
#include <memory>
#include <numeric>
#include <random>
#include <sstream>
#include <stdexcept>
#include <utility>
#include <vector>

namespace fetch {
namespace math {

template <typename T, typename C>
class Tensor
{
public:
  using Type                       = T;
  using ContainerType              = C;
  using VectorSliceType            = typename ContainerType::VectorSliceType;
  using VectorRegisterType         = typename ContainerType::VectorRegisterType;
  using VectorRegisterIteratorType = typename ContainerType::VectorRegisterIteratorType;

  using IteratorType      = TensorIterator<T>;
  using ConstIteratorType = ConstTensorIterator<T>;

  using SliceIteratorType      = TensorSliceIterator<T, ContainerType>;
  using ConstSliceIteratorType = ConstTensorSliceIterator<T, ContainerType>;
  using ViewType               = TensorView<T, C>;
  using SizeType               = fetch::math::SizeType;
  using SizeVector             = fetch::math::SizeVector;

  static constexpr char const *LOGGING_NAME = "Tensor";

  enum
  {
    LOG_PADDING = ViewType::LOG_PADDING,
    PADDING     = ViewType::PADDING
  };

private:
  template <typename STensor>
  class TensorSliceImplementation;
  template <SizeType N, typename TSType, typename... Args>
  struct TensorSetter;
  template <SizeType N, typename TSType>
  struct TensorSetter<N, TSType>;

public:
  using ConstSliceType = TensorSliceImplementation<Tensor const>;

  class TensorSlice;

  using SliceType = TensorSlice;

  enum MAJOR_ORDER
  {
    COLUMN,
    ROW
  };

  Tensor()
  {
    Resize({0});
  }

  //  explicit Tensor(ContainerType &container_data)
  //  {
  //    Reshape(container_data.shape());
  //    data_ = container_data;
  //  }

  static Tensor FromString(byte_array::ConstByteArray const &c);
  explicit Tensor(SizeType const &n);
  Tensor(Tensor &&other) noexcept = default;
  Tensor(Tensor const &other)     = default;
  explicit Tensor(SizeVector const &dims);
  virtual ~Tensor() = default;

  Tensor &operator=(Tensor const &other) = default;
  Tensor &operator=(Tensor &&other) noexcept = default;

  IteratorType      begin();
  IteratorType      end();
  ConstIteratorType begin() const;
  ConstIteratorType end() const;
  ConstIteratorType cbegin() const;
  ConstIteratorType cend() const;

  ////////////////////////////////
  /// ASSIGNMENT AND ACCESSING ///
  ////////////////////////////////

  void   Copy(Tensor const &x);
  Tensor Copy() const;
  template <typename G>
  void Assign(TensorSliceImplementation<G> const &other);
  void Assign(TensorSlice const &other);
  void Assign(Tensor const &other);
  void Assign(TensorView<T, C> const &other);

  template <typename... Indices>
  Type &At(Indices... indices);
  template <typename... Indices>
  Type At(Indices... indices) const;

  template <typename... Indices>
  Type operator()(Indices... indices) const;
  template <typename... Indices>
  Type &operator()(Indices... indices);

  template <typename... Args>
  void Set(Args... args);

  Type operator()(SizeType const &index) const;
  template <typename S>
  std::enable_if_t<std::is_integral<S>::value, Type> &operator[](S const &n);
  template <typename S>
  std::enable_if_t<std::is_integral<S>::value, Type> const &operator[](S const &i) const;

  Tensor &operator=(ConstSliceType const &slice);
  Tensor &operator=(TensorSlice const &slice);

  void Fill(Type const &value, memory::Range const &&range);
  void Fill(Type const &value);
  void SetAllZero();
  void SetAllOne();
  void SetPaddedZero();

  ContainerType const &data() const;
  ContainerType &      data();

  Tensor FillArange(Type const &from, Type const &to);

  static Tensor UniformRandom(SizeType const &N);
  static Tensor UniformRandomIntegers(SizeType const &N, int64_t min, int64_t max);
  Tensor &      FillUniformRandom();
  Tensor &      FillUniformRandomIntegers(int64_t min, int64_t max);
  static Tensor Zeroes(SizeVector const &shape);
  static Tensor Ones(SizeVector const &shape);
  SizeType      ComputeIndex(SizeVector const &indices) const;

  ////////////////////
  /// SHAPE & SIZE ///
  ////////////////////

  static SizeType SizeFromShape(SizeVector const &shape);
  static SizeType PaddedSizeFromShape(SizeVector const &shape);

  void    Flatten();
  Tensor  Transpose() const;  // TODO (private 867)
  Tensor  Transpose(SizeVector &new_axes) const;
  Tensor &Squeeze();
  Tensor &Unsqueeze();

  /////////////////////////
  /// memory management ///
  /////////////////////////

  bool Resize(SizeVector const &shape, bool copy = false);
  bool Reshape(SizeVector const &shape);

  SizeVector const &stride() const;
  SizeVector const &shape() const;
  SizeType const &  shape(SizeType const &n) const;
  SizeType          size() const;

  template <typename S>
  fetch::meta::IfIsUnsignedInteger<S, void> Set(std::vector<S> const &indices, Type const &val);

  template <typename S>
  fetch::meta::IfIsUnsignedInteger<S, Type> Get(std::vector<S> const &indices) const;

  ///////////////////////
  /// MATH OPERATIONS ///
  ///////////////////////

  Tensor InlineAdd(Tensor const &other);
  Tensor InlineAdd(Type const &scalar);
  Tensor InlineSubtract(Tensor const &other);
  Tensor InlineSubtract(Type const &scalar);
  Tensor InlineReverseSubtract(Tensor const &other);
  Tensor InlineReverseSubtract(Type const &scalar);
  Tensor InlineMultiply(Tensor const &other);
  Tensor InlineMultiply(Type const &scalar);
  Tensor InlineDivide(Tensor const &other);
  Tensor InlineDivide(Type const &scalar);
  Tensor InlineReverseDivide(Tensor const &other);
  Tensor InlineReverseDivide(Type const &scalar);

  template <typename OtherType>
  Tensor operator+(OtherType const &other);

  template <typename OtherType>
  Tensor operator+=(OtherType const &other);

  template <typename OtherType>
  Tensor operator-(OtherType const &other);

  template <typename OtherType>
  Tensor operator-=(OtherType const &other);

  template <typename OtherType>
  Tensor operator*(OtherType const &other);

  template <typename OtherType>
  Tensor operator*=(OtherType const &other);

  template <typename OtherType>
  Tensor operator/(OtherType const &other);

  template <typename OtherType>
  Tensor operator/=(OtherType const &other);

  /////////////
  /// Order ///
  /////////////

  void        MajorOrderFlip();
  MAJOR_ORDER MajorOrder() const;

  //////////////
  /// Slices ///
  //////////////

  ConstSliceType Slice() const;
  ConstSliceType Slice(SizeType index, SizeType axis = 0) const;
  ConstSliceType Slice(SizeVector indices, SizeVector axes) const;
  ConstSliceType Slice(SizeVector const &begins, SizeVector const &ends,
                       SizeVector const &strides) const;
  TensorSlice    Slice();
  TensorSlice    Slice(SizeType index, SizeType axis = 0);
  TensorSlice    Slice(std::pair<SizeType, SizeType> start_end_index, SizeType axis = 0);
  TensorSlice    Slice(SizeVector indices, SizeVector axes);
  TensorSlice    Slice(SizeVector const &begins, SizeVector const &ends, SizeVector const &strides);

  /////////////
  /// Views ///
  /////////////
  TensorView<Type, ContainerType>       View();
  TensorView<Type, ContainerType> const View() const;
  TensorView<Type, ContainerType>       View(SizeType index);
  TensorView<Type, ContainerType> const View(SizeType index) const;
  TensorView<Type, ContainerType>       View(std::vector<SizeType> indices);
  TensorView<Type, ContainerType> const View(std::vector<SizeType> indices) const;

  /////////////////////////
  /// general utilities ///
  /////////////////////////

  std::string ToString() const;
  SizeType    Find(Type val) const;

  template <typename TensorType>
  static Tensor              Stack(std::vector<TensorType> const &tensors);
  static Tensor              Concat(std::vector<Tensor> const &tensors, SizeType axis);
  static std::vector<Tensor> Split(Tensor const &tensor, SizeVector const &concat_points,
                                   SizeType axis);

  void Sort();
  void Sort(memory::Range const &range);

  static Tensor Arange(Type const &from, Type const &to, Type const &delta);

  ////////////////////////////
  /// COMPARISON OPERATORS ///
  ////////////////////////////

  bool AllClose(Tensor const &o,
                Type const &  relative_tolerance = fetch::math::Type<Type>("0.00001"),
                Type const &  absolute_tolerance = fetch::math::Type<Type>("0.00000001")) const;
  bool operator==(Tensor const &other) const;
  bool operator!=(Tensor const &other) const;

  //////////////////
  /// TensorSlice///
  //////////////////

  /**
   * TensorSlice class contains the non-const methods of tensor slicing
   */
  class TensorSlice : public TensorSliceImplementation<Tensor>
  {
  public:
    using Type = T;
    using TensorSliceImplementation<Tensor>::TensorSliceImplementation;
    using TensorSliceImplementation<Tensor>::cbegin;
    using TensorSliceImplementation<Tensor>::cend;
    using TensorSliceImplementation<Tensor>::Slice;

    SliceIteratorType begin();
    SliceIteratorType end();
    TensorSlice       Slice(SizeType index, SizeType axis);
    void              ModifyRange(SizeType i, SizeType axis);

    template <typename G>
    void Assign(TensorSliceImplementation<G> const &other);
    void Assign(Tensor const &other);
    void Fill(Type t);
  };

  ////////////////////////////////
  /// Serialization operations ///
  ////////////////////////////////
  template <typename A, typename B>
  friend struct serializers::MapSerializer;

  // TODO(private 858): Vectorize and deduce D from parent
  template <typename S, typename D = memory::SharedArray<S>>
  void As(Tensor<S, D> &ret) const
  {
    ret.Resize({size_});
    auto this_it = cbegin();
    auto ret_it  = begin();

    while (this_it.is_valid())
    {
      *ret_it = *this_it;
      ++ret_it;
      ++this_it;
    }
  }

  /////////////////////////////
  /// Convenience functions ///
  /////////////////////////////

  SizeType height() const
  {
    return shape_[0];
  }

  SizeType width() const
  {
    return (shape_.size() > 1 ? shape_[1] : 1);
  }

  SizeType depth() const
  {
    return (shape_.size() > 2 ? shape_[2] : 1);
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

  bool IsVector() const
  {
    return shape_.size() == 1;
  }

  bool IsMatrix() const
  {
    return shape_.size() == 2;
  }

private:
  ContainerType data_{};
  SizeType      size_{0};
  SizeVector    shape_{};
  SizeVector    stride_{};
  SizeType      padded_height_{};

  MAJOR_ORDER major_order_{COLUMN};

  /**
   * Gets a value from the array by N-dim index
   * @param indices index to access
   */
  // TODO (private 868):
  template <SizeType N, typename FirstIndex, typename... Indices>
  SizeType UnrollComputeColIndex(FirstIndex &&index, Indices &&... indices) const
  {
    if (shape_.at(N) <= SizeType(index))
    {
      throw exceptions::WrongIndices(
          "Tensor::At : index " + std::to_string(SizeType(index)) + " is out of bounds of axis " +
          std::to_string(N) + " (max possible index is " + std::to_string(shape_.at(N) - 1) + ").");
    }
    return static_cast<SizeType>(index) * stride_.at(N) +
           UnrollComputeColIndex<N + 1>(std::forward<Indices>(indices)...);
  }

  template <SizeType N, typename FirstIndex>
  SizeType UnrollComputeColIndex(FirstIndex &&index) const
  {
    if (shape_.at(N) <= SizeType(index))
    {
      throw exceptions::WrongIndices(
          "Tensor::At : index " + std::to_string(SizeType(index)) + " is out of bounds of axis " +
          std::to_string(N) + " (max possible index is " + std::to_string(shape_.at(N) - 1) + ").");
    }
    return static_cast<SizeType>(index) * stride_.at(N);
  }

  void UpdateStrides()
  {
    stride_.resize(shape_.size());
    SizeType n_dims = shape_.size();
    SizeType base   = padded_height_;

    stride_[0] = 1;
    for (SizeType i = 1; i < n_dims; ++i)
    {
      stride_[i] = base;
      base *= shape_[i];
    }

    // TODO (private 870): Reverse order if row major.
  }

  // TODO(private 871): replace with strides
  /*
  SizeType ComputeRowIndex(SizeVector const &indices) const
  {
    SizeType index  = 0;
    SizeType n_dims = indices.size();
    SizeType base   = 1;

    // loop through all dimensions
    for (SizeType i = n_dims; i == 0; --i)
    {
      index += indices[i - 1] * base;
      base *= shape_[i - 1];
    }
    return index;
  }
  */

  template <typename S>
  fetch::meta::IfIsUnsignedInteger<S, SizeType> ComputeColIndex(std::vector<S> const &indices) const
  {
    assert(!indices.empty());
    SizeType index  = indices[0];
    SizeType n_dims = indices.size();
    SizeType base   = padded_height_;

    // loop through all dimensions
    for (SizeType i = 1; i < n_dims; ++i)
    {
      index += indices[i] * base;
      base *= shape_[i];
    }
    return index;
  }

  /**
   * rearranges data storage in the array. Slow because it copies data instead of pointers
   * @param major_order
   */
  void FlipMajorOrder(MAJOR_ORDER major_order)
  {
    Tensor new_array{this->shape()};

    SizeVector stride;
    SizeVector index;

    SizeType cur_stride = Product(this->shape());

    for (SizeType i = 0; i < new_array.shape().size(); ++i)
    {
      cur_stride /= this->shape()[i];
      stride.emplace_back(cur_stride);
      index.emplace_back(0);
    }

    SizeType          total_size = Tensor::SizeFromShape(new_array.shape());
    SliceIteratorType it_this(*this);

    SizeType cur_dim;
    SizeType pos;

    if (major_order == MAJOR_ORDER::COLUMN)
    {
      new_array.Copy(*this);
    }

    for (SizeType j = 0; j < total_size; ++j)
    {
      // Compute current row major index
      pos = 0;
      for (cur_dim = 0; cur_dim < this->shape().size(); ++cur_dim)
      {
        pos += stride[cur_dim] * index[cur_dim];
      }
      assert(pos < total_size);

      // copy the data
      if (major_order == MAJOR_ORDER::ROW)
      {
        new_array[pos] = *it_this;
      }
      else
      {
        *it_this = new_array[pos];
      }
      ++it_this;

      // Incrementing current dimensions and index as necessary
      cur_dim = 0;
      ++index[cur_dim];

      while (index[cur_dim] >= this->shape()[cur_dim])
      {
        index[cur_dim] = 0;
        ++cur_dim;
        if (cur_dim >= this->shape().size())
        {
          break;
        }
        ++index[cur_dim];
      }
    }

    if (major_order == MAJOR_ORDER::ROW)
    {
      this->Copy(new_array);
    }

    if (major_order == MAJOR_ORDER::COLUMN)
    {
      major_order_ = MAJOR_ORDER::COLUMN;
    }
    else
    {
      major_order_ = MAJOR_ORDER::ROW;
    }
  }

  void TransposeImplementation(SizeVector &new_axes, Tensor &ret) const
  {
    ConstSliceIteratorType it(*this);
    SliceIteratorType      ret_it(ret);

    ret_it.Transpose(new_axes);

    while (it.is_valid())
    {
      *ret_it = *it;
      ++it;
      ++ret_it;
    }
  }

  /**
   * The TensorSlice is a convenience method for efficiently manipulating
   * SubTensors (e.g. such as a 1D Slice). It is built on top of TensorSliceIterator
   * @tparam STensor
   */
  template <typename STensor>
  class TensorSliceImplementation
  {
  public:
    using Type = typename STensor::Type;

    /**
     * Construct TensorSlice with multiple axes
     * @param t Original tensor
     * @param range
     * @param axes
     */
    TensorSliceImplementation<STensor>(STensor &t, std::vector<SizeVector> range, SizeVector axes)
      : tensor_{t}
      , range_{std::move(range)}
      , axes_{std::move(axes)}
    {}

    /**
     * Construct TensorSlice with single axis
     * @param t Original tensor
     * @param range
     * @param axis
     */
    TensorSliceImplementation<STensor>(STensor &t, std::vector<SizeVector> range, SizeType axis = 0)
      : tensor_{t}
      , range_{std::move(range)}
      , axis_{axis}
    {}

    std::string ToString() const
    {
      return Copy().ToString();
    }

    Tensor                 Copy() const;
    ConstSliceType         Slice(SizeType i, SizeType axis) const;
    void                   ModifyRange(SizeType i, SizeType axis);
    ConstSliceIteratorType cbegin() const;
    ConstSliceIteratorType cend() const;
    SizeType               size() const;
    SizeVector             shape() const;

  protected:
    STensor &               tensor_;
    std::vector<SizeVector> range_;
    std::vector<SizeType>   axes_;
    SizeType                axis_{};
  };
};

//////////////////////////////
/// ASSIGNMENT & ACCESSING ///
//////////////////////////////

/**
 * Method assigns the data from the other tensor slice into this tensor
 * @tparam G
 */
template <typename T, typename C>
template <typename G>
void Tensor<T, C>::Assign(TensorSliceImplementation<G> const &other)
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
 * Tensor Accessor with shape().size() many indices
 * @tparam T Type
 * @tparam C Container
 * @tparam Indices parameter pack of indices
 * @param indices Indices for accessing the data
 * @return Returns the value in the Tensor at the location specified
 */
template <typename T, typename C>
template <typename... Indices>
typename Tensor<T, C>::Type &Tensor<T, C>::At(Indices... indices)
{
  if (sizeof...(indices) != stride_.size())
  {
    throw exceptions::WrongIndices(
        "Wrong arguments quantity (" + std::to_string(sizeof...(indices)) +
        ") given to Tensor::At, expected: " + std::to_string(stride_.size()));
  }
  return this->data()[UnrollComputeColIndex<0>(std::forward<Indices>(indices)...)];
}

/**
 * Tensor Accessor with shape().size() many indices
 * @tparam T Type
 * @tparam C Container
 * @tparam Indices parameter pack of indices
 * @param indices Indices for accessing the data
 * @return Returns the value in the Tensor at the location specified
 */
template <typename T, typename C>
template <typename... Indices>
typename Tensor<T, C>::Type Tensor<T, C>::At(Indices... indices) const
{
  if (sizeof...(indices) != stride_.size())
  {
    throw exceptions::WrongIndices(
        "Wrong arguments quantity (" + std::to_string(sizeof...(indices)) +
        ") given to Tensor::At, expected: " + std::to_string(stride_.size()));
  }
  SizeType N = UnrollComputeColIndex<0>(std::forward<Indices>(indices)...);
  return this->data()[N];
}

/**
 * Operator for accessing data in the tensor
 * @tparam T Type
 * @tparam C Container
 * @tparam Indices parameter pack of indices
 * @param indices Indices for accessing the data
 * @return Returns the value in tensor at the location specified
 */
template <typename T, typename C>
template <typename... Indices>
typename Tensor<T, C>::Type Tensor<T, C>::operator()(Indices... indices) const
{
  return At(std::forward<Indices>(indices)...);
}

/**
 * Operator for accessing data in the tensor
 * @tparam T Type
 * @tparam C Container
 * @tparam Indices parameter pack of indices
 * @param indices Indices for accessing the data
 * @return Returns the value in tensor at the location specified
 */
template <typename T, typename C>
template <typename... Indices>
typename Tensor<T, C>::Type &Tensor<T, C>::operator()(Indices... indices)
{
  return At(std::forward<Indices>(indices)...);
}

/**
 * One-dimensional reference index operator
 * This operator acts as a one-dimensional array accessor that is
 * meant for non-constant object instances.
 * Method restricted to Integral types for indexing.
 * @tparam T Type
 * @tparam C Container
 * @tparam S Integral type for accessing
 * @param index index to access tensor
 * @return data stored at indexed location
 */
template <typename T, typename C>
template <typename S>
std::enable_if_t<std::is_integral<S>::value, typename Tensor<T, C>::Type> &Tensor<T, C>::operator[](
    S const &n)
{
  assert(static_cast<SizeType>(n) < size());
  if (shape_.size() == 1)
  {
    return data_[n];
  }

  SizeType j = static_cast<SizeType>(n) / height();
  SizeType i = static_cast<SizeType>(n) - j * height();

  assert(i + padded_height_ * j < padded_size());
  return data_[i + padded_height_ * j];
}

/**
 * One-dimensional reference index operator
 * This operator acts as a one-dimensional array accessor that is
 * meant for non-constant object instances.
 * Method restricted to Integral types for indexing.
 * @tparam T Type
 * @tparam C Container
 * @tparam S Integral type for accessing
 * @param index index to access tensor
 * @return data stored at indexed location
 */
template <typename T, typename C>
template <typename S>
std::enable_if_t<std::is_integral<S>::value, typename Tensor<T, C>::Type> const
    &Tensor<T, C>::operator[](S const &i) const
{
  return data_[i];
}

/**
 * Set operator takes variable number of indices followed by one value.
 * This is made possible using the TensorSetter class to manage
 * template unrolling
 *
 * @tparam T Type
 * @tparam C Container
 * @tparam Args indices followed by one value to set
 * @param args indices followed by one value to set
 */
template <typename T, typename C>
template <typename... Args>
void Tensor<T, C>::Set(Args... args)
{
  // Plus one as last arg is value
  if (sizeof...(args) != (stride_.size() + 1))
  {
    throw exceptions::WrongIndices(
        "Wrong arguments quantity (" + std::to_string(sizeof...(args)) +
        ") given to Tensor::Set, expected: " + std::to_string(stride_.size() + 1));
  }

  uint64_t index = TensorSetter<0, Args...>::IndexOf(stride_, shape_, std::forward<Args>(args)...);
  Type     value = TensorSetter<0, Args...>::ValueOf(std::forward<Args>(args)...);

  data_[index] = std::move(value);
}

/**
 * Sets a single value in the array using an n-dimensional index
 * @param indices     index position in array
 * @param val         value to write
 */
// TODO(private issue 123)
template <typename T, typename C>
template <typename S>
fetch::meta::IfIsUnsignedInteger<S, void> Tensor<T, C>::Set(std::vector<S> const &indices,
                                                            Type const &          val)
{
  if (indices.size() != shape_.size())
  {
    throw exceptions::WrongIndices(
        "Wrong indices quantity (" + std::to_string(indices.size()) +
        ") given to Tensor::Set, expected: " + std::to_string(shape_.size()));
  }

  this->operator[](ComputeColIndex(indices)) = val;
}

/**
 * Gets a value from the array by N-dim index
 * @param indices index to access
 */
template <typename T, typename C>
template <typename S>
fetch::meta::IfIsUnsignedInteger<S, typename Tensor<T, C>::Type> Tensor<T, C>::Get(
    std::vector<S> const &indices) const
{
  assert(indices.size() == shape_.size());
  return this->operator[](ComputeColIndex(indices));
}

///////////////////////
/// MATH OPERATIONS ///
///////////////////////

/**
 * + operator
 * @tparam OtherType may be a scalar or array, but must be arithmetic
 * @param other
 * @return returns this Tensor after add operation
 */
template <typename T, typename C>
template <typename OtherType>
Tensor<T, C> Tensor<T, C>::operator+(OtherType const &other)
{
  Tensor<T, C> ret{this->shape()};
  Add(*this, other, ret);
  return ret;
}

template <typename T, typename C>
template <typename OtherType>
Tensor<T, C> Tensor<T, C>::operator+=(OtherType const &other)
{
  return InlineAdd(other);
}

/**
 * + operator
 * @tparam OtherType may be a scalar or array, but must be arithmetic
 * @param other
 * @return returns this Tensor after subtract operation
 */
template <typename T, typename C>
template <typename OtherType>
Tensor<T, C> Tensor<T, C>::operator-(OtherType const &other)
{
  Tensor<T, C> ret{this->shape()};
  Subtract(*this, other, ret);
  return ret;
}

template <typename T, typename C>
template <typename OtherType>
Tensor<T, C> Tensor<T, C>::operator-=(OtherType const &other)
{
  return InlineSubtract(other);
}

/**
 * * operator
 * @tparam OtherType may be a scalar or array, but must be arithmetic
 * @param other
 * @return
 */
template <typename T, typename C>
template <typename OtherType>
Tensor<T, C> Tensor<T, C>::operator*(OtherType const &other)
{
  Tensor<T, C> ret(this->shape());
  Multiply(*this, other, ret);
  return ret;
}

template <typename T, typename C>
template <typename OtherType>
Tensor<T, C> Tensor<T, C>::operator*=(OtherType const &other)
{
  return InlineMultiply(other);
}

template <typename T, typename C>
template <typename OtherType>
Tensor<T, C> Tensor<T, C>::operator/(OtherType const &other)
{
  Tensor<T, C> ret(this->shape());
  Divide(*this, other, ret);
  return ret;
}

template <typename T, typename C>
template <typename OtherType>
Tensor<T, C> Tensor<T, C>::operator/=(OtherType const &other)
{
  return InlineDivide(other);
}

/////////////////////////
/// GENERAL UTILITIES ///
/////////////////////////

/**
 * Stack tensors resulting in a new leading dimension
 * @tparam Tensor
 * @param tensors
 * @return
 */
template <typename T, typename C>
template <typename TensorType>
Tensor<T, C> Tensor<T, C>::Stack(std::vector<TensorType> const &tensors)
{
  SizeVector ret_size;
  ret_size.emplace_back(tensors.size());
  ret_size.insert(ret_size.end(), tensors.front().shape().begin(), tensors.front().shape().end());
  TensorType ret(ret_size);
  for (SizeType i(0); i < tensors.size(); ++i)
  {
    ret.Slice(i).Assign(tensors[i]);
  }
  return ret;
}

/////////////////////
/// Tensor Setter ///
/////////////////////

/**
 * The Tensor Setter handles template unrolling for the Args parameter pack which ends in one value
 * @tparam T Type
 * @tparam C Container
 * @tparam N The index of the dimension currently being unrolled
 * @tparam TSType Tensor SizeType
 * @tparam Args The Parameter pack containing the indices and one value
 */
template <typename T, typename C>
template <SizeType N, typename TSType, typename... Args>
struct Tensor<T, C>::TensorSetter
{
  // Finding the return value
  using Type = typename TensorSetter<N + 1, Args...>::Type;

  // Computing index
  static SizeType IndexOf(SizeVector const &stride, SizeVector const &shape, TSType const &index,
                          Args &&... args)
  {
    if (SizeType(index) >= shape[N])
    {
      throw exceptions::WrongIndices("Tensor::IndexOf : index " + std::to_string(SizeType(index)) +
                                     " is out of bounds of axis " + std::to_string(N) +
                                     " (max possible index is " + std::to_string(shape[N] - 1) +
                                     ").");
    }
    return stride[N] * SizeType(index) +
           TensorSetter<N + 1, Args...>::IndexOf(stride, shape, std::forward<Args>(args)...);
  }

  // Ignoring all arguments but the last
  static Type ValueOf(TSType const &index, Args &&... args)
  {
    FETCH_UNUSED(index);
    return TensorSetter<N + 1, Args...>::ValueOf(std::forward<Args>(args)...);
  }
};

/**
 * The limit case of the template unrolling for Set
 * @tparam T Type
 * @tparam C Container
 * @tparam N The index of the dimension currently being unrolled
 * @tparam TSType Tensor SizeType
 */
template <typename T, typename C>
template <SizeType N, typename TSType>
struct Tensor<T, C>::TensorSetter<N, TSType>
{
  using Type = TSType;

  // Ignore last argument (i.e. value)
  static SizeType IndexOf(SizeVector const &stride, SizeVector const &shape, TSType const &index)
  {
    assert(shape.size() == N);
    assert(stride.size() == N);
    FETCH_UNUSED(index);
    FETCH_UNUSED(stride);
    FETCH_UNUSED(shape);
    return 0;
  }

  // Extracting last argument
  static TSType ValueOf(TSType const &value)
  {
    return value;
  }
};

///////////////////////////////////////////
/// TENSOR SLICE METHOD IMPLEMENTATIONS ///
///////////////////////////////////////////

// TensorSlice implementations

template <typename T, typename C>
typename Tensor<T, C>::SliceIteratorType Tensor<T, C>::TensorSlice::begin()
{
  auto ret = SliceIteratorType(this->tensor_, this->range_);

  if (this->axes_.empty())
  {
    if (this->axis_ != 0)
    {
      ret.MoveAxisToFront(this->axis_);
    }
  }
  else
  {
    // If there is only one axis and is 0, it's already at front
    if ((this->axes_.size() != 1) || (this->axes_.at(0) != 0))
    {
      ret.MoveAxesToFront(this->axes_);
    }
  }

  return ret;
}

template <typename T, typename C>
typename Tensor<T, C>::SliceIteratorType Tensor<T, C>::TensorSlice::end()
{
  return SliceIteratorType::EndIterator(this->tensor_);
}

/**
 * Returns a Slice of a Slice
 * @tparam T Slice Type
 * @tparam C Slice ContainerType
 * @tparam STensor original tensor type
 * @param index offset
 * @param axis
 * @return
 */
template <typename T, typename C>
typename Tensor<T, C>::TensorSlice Tensor<T, C>::TensorSlice::Slice(SizeType index, SizeType axis)
{
  std::vector<SizeType> new_axes(this->axes_);

  // If new axes are empty, it means that there was single axis
  if (new_axes.empty())
  {
    new_axes.emplace_back(this->axis_);
  }

  // Test validity
  assert(axis < this->tensor_.shape().size());
  assert(new_axes.size() < this->tensor_.shape().size());
  assert(index < this->tensor_.shape().at(axis));
  for (SizeType new_axis : new_axes)
  {
    FETCH_UNUSED(new_axis);
    assert(new_axis != axis);
  }

  std::vector<SizeVector> new_range(this->range_);

  // Modify range based on specified offset i
  new_range.at(axis).at(0) = index;
  new_range.at(axis).at(1) = index + 1;
  new_range.at(axis).at(2) = 1;

  // Add new axis
  new_axes.emplace_back(axis);

  return TensorSlice(this->tensor_, new_range, new_axes);
}

template <typename T, typename C>
void Tensor<T, C>::TensorSlice::ModifyRange(SizeType i, SizeType axis)
{
  assert(axis < this->tensor_.shape().size());
  assert(i < this->tensor_.shape().at(axis));

  // Modify range based on specified offset i
  this->range_.at(axis).at(0) = i;
  this->range_.at(axis).at(1) = i + 1;
  this->range_.at(axis).at(2) = 1;
}

template <typename T, typename C>
template <typename G>
void Tensor<T, C>::TensorSlice::Assign(TensorSliceImplementation<G> const &other)
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

template <typename T, typename C>
void Tensor<T, C>::TensorSlice::Assign(Tensor const &other)
{
  auto it1 = begin();
  auto it2 = other.begin();
  assert(it1.size() == it2.size());
  while (it1.is_valid())
  {
    *it1 = *it2;
    ++it1;
    ++it2;
  }
}

template <typename T, typename C>
void Tensor<T, C>::TensorSlice::Fill(Type t)
{
  auto it = begin();
  while (it.is_valid())
  {
    *it = t;
    ++it;
  }
}

// TensorSliceImplementation implementations

template <typename T, typename C>
template <typename STensor>
Tensor<T, C> Tensor<T, C>::TensorSliceImplementation<STensor>::Copy() const
{
  SizeVector shape;
  for (SizeType i{0}; i < this->range_.size(); ++i)
  {
    shape.emplace_back(((this->range_[i][1] - this->range_[i][0] - 1) / this->range_[i][2]) + 1);
  }
  fetch::math::Tensor<T, C> ret{shape};
  ret.Assign(*this);
  return ret;
}

/**
 * Returns a ConstSlice of a ConstSlice that is not permitted to alter the original tensor
 * @tparam T Slice Type
 * @tparam C Slice ContainerType
 * @tparam STensor original tensor type
 * @param index offset
 * @param axis
 * @return
 */
template <typename T, typename C>
template <typename STensor>
typename Tensor<T, C>::ConstSliceType Tensor<T, C>::TensorSliceImplementation<STensor>::Slice(
    SizeType i, SizeType axis) const
{
  std::vector<SizeType> new_axes(axes_);

  // If new axes are empty, it means that there was single axis
  if (axes_.empty())
  {
    new_axes.emplace_back(axis_);
  }

  // Test validity
  assert(axis < tensor_.shape().size());
  assert(new_axes.size() < tensor_.shape().size());
  assert(i < tensor_.shape().at(axis));
  for (SizeType new_axis : new_axes)
  {
    FETCH_UNUSED(new_axis);
    assert(new_axis != axis);
  }

  std::vector<SizeVector> new_range(range_);

  // Modify range based on specified offset i
  new_range.at(axis).at(0) = i;
  new_range.at(axis).at(1) = i + 1;
  new_range.at(axis).at(2) = 1;

  // Add new axis
  new_axes.emplace_back(axis);

  return ConstSliceType(tensor_, new_range, new_axes);
}

template <typename T, typename C>
template <typename STensor>
void Tensor<T, C>::TensorSliceImplementation<STensor>::ModifyRange(SizeType i, SizeType axis)
{
  assert(axis < this->tensor_.shape().size());
  assert(i < this->tensor_.shape().at(axis));

  // Modify range based on specified offset i
  this->range_.at(axis).at(0) = i;
  this->range_.at(axis).at(1) = i + 1;
  this->range_.at(axis).at(2) = 1;
}

template <typename T, typename C>
template <typename STensor>
typename Tensor<T, C>::ConstSliceIteratorType
Tensor<T, C>::TensorSliceImplementation<STensor>::cbegin() const
{
  auto ret = ConstSliceIteratorType(tensor_, range_);

  // axis_ is used when using only one axis
  if (this->axes_.empty())
  {
    if (this->axis_ != 0)
    {
      ret.MoveAxisToFront(this->axis_);
    }
    // axes_ is used when using more than one axis
  }
  else
  {
    // If there is only one axis and is 0, it's already at front
    if ((this->axes_.size() != 1) || (this->axes_.at(0) != 0))
    {
      ret.MoveAxesToFront(this->axes_);
    }
  }

  return ret;
}

template <typename T, typename C>
template <typename STensor>
typename Tensor<T, C>::ConstSliceIteratorType
Tensor<T, C>::TensorSliceImplementation<STensor>::cend() const
{
  return ConstSliceIteratorType::EndIterator(tensor_);
}

template <typename T, typename C>
template <typename STensor>
typename Tensor<T, C>::SizeType Tensor<T, C>::TensorSliceImplementation<STensor>::size() const
{
  return tensor_.size();
}

template <typename T, typename C>
template <typename STensor>
typename Tensor<T, C>::SizeVector Tensor<T, C>::TensorSliceImplementation<STensor>::shape() const
{
  return tensor_.shape();
}

}  // namespace math

namespace serializers {

template <typename V, typename D>
struct ArraySerializer<memory::SharedArray<V>, D>
{
public:
  using Type       = memory::SharedArray<V>;
  using DriverType = D;

  template <typename Constructor>
  static void Serialize(Constructor &array_constructor, Type const &input)
  {
    auto array = array_constructor(input.size());
    for (uint32_t i = 0; i < input.size(); ++i)
    {
      array.Append(input[i]);
    }
  }

  template <typename ArrayDeserializer>
  static void Deserialize(ArrayDeserializer &array, Type &output)
  {
    output = Type(array.size());
    for (uint32_t i = 0; i < output.size(); ++i)
    {
      array.GetNextValue(output[i]);
    }
  }
};

template <typename V, typename D>
struct ArraySerializer<memory::Array<V>, D>
{
public:
  using Type       = memory::Array<V>;
  using DriverType = D;

  template <typename Constructor>
  static void Serialize(Constructor &array_constructor, Type const &input)
  {
    auto array = array_constructor(input.size());
    for (uint32_t i = 0; i < input.size(); ++i)
    {
      array.Append(input[i]);
    }
  }

  template <typename ArrayDeserializer>
  static void Deserialize(ArrayDeserializer &array, Type &output)
  {
    output = Type(array.size());
    for (uint32_t i = 0; i < output.size(); ++i)
    {
      array.GetNextValue(output[i]);
    }
  }
};

template <typename A, typename B, typename D>
struct MapSerializer<math::Tensor<A, B>, D>
{
public:
  using Type       = math::Tensor<A, B>;
  using DriverType = D;

  static uint8_t const DATA          = 1;
  static uint8_t const SIZE          = 2;
  static uint8_t const SHAPE         = 3;
  static uint8_t const STRIDE        = 4;
  static uint8_t const PADDED_HEIGHT = 5;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &tensor)
  {
    auto map = map_constructor(5);
    map.Append(DATA, tensor.data_);
    map.Append(SIZE, tensor.size_);
    map.Append(SHAPE, tensor.shape_);
    map.Append(STRIDE, tensor.stride_);
    map.Append(PADDED_HEIGHT, tensor.padded_height_);
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &tensor)
  {
    map.ExpectKeyGetValue(DATA, tensor.data_);
    map.ExpectKeyGetValue(SIZE, tensor.size_);
    map.ExpectKeyGetValue(SHAPE, tensor.shape_);
    map.ExpectKeyGetValue(STRIDE, tensor.stride_);
    map.ExpectKeyGetValue(PADDED_HEIGHT, tensor.padded_height_);
  }
};

}  // namespace serializers

}  // namespace fetch
