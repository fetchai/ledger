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

#include "core/byte_array/const_byte_array.hpp"
#include "core/byte_array/consumers.hpp"
#include "core/macros.hpp"
#include "core/random.hpp"
#include "core/serializers/group_definitions.hpp"
#include "math/activation_functions/softmax.hpp"
#include "math/base_types.hpp"
#include "math/matrix_operations.hpp"
#include "math/metrics/l2_loss.hpp"
#include "math/metrics/l2_norm.hpp"
#include "math/standard_functions/abs.hpp"
#include "math/standard_functions/exp.hpp"
#include "math/standard_functions/fmod.hpp"
#include "math/standard_functions/remainder.hpp"
#include "math/tensor/tensor_broadcast.hpp"
#include "math/tensor/tensor_iterator.hpp"
#include "math/tensor/tensor_slice_iterator.hpp"
#include "math/tensor/tensor_view.hpp"
#include "vectorise/memory/array.hpp"

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

class TensorBase
{
public:
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

  explicit Tensor(ContainerType &container_data)
  {
    Reshape(container_data.shape());
    data_ = container_data;
  }

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

  void Fill(Type const &value, memory::Range const &range);
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

  // TODO (issue 1117): Make free functions
  Type Sum() const;
  void Exp(Tensor const &x);
  void ApproxSoftMax(Tensor const &x);
  Type L2Norm() const;
  Type L2Loss() const;

  Type   PeakToPeak() const;
  void   Fmod(Tensor const &x);
  void   Remainder(Tensor const &x);
  Tensor Softmax(Tensor const &x);

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

}  // namespace math
}  // namespace fetch
