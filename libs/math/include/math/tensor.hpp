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
#include "math/tensor_broadcast.hpp"
#include "math/tensor_iterator.hpp"
#include "math/tensor_slice_iterator.hpp"
#include "math/tensor_view.hpp"
#include "vectorise/memory/array.hpp"

#include <cassert>
#include <cstdint>
#include <memory>
#include <numeric>
#include <random>
#include <stdexcept>
#include <utility>
#include <vector>

namespace fetch {
namespace math {

namespace details {
template <typename DataType, typename ArrayType>
static void ArangeImplementation(DataType const &from, DataType const &to, DataType const &delta,
                                 ArrayType &ret)
{
  auto N = SizeType((to - from) / delta);
  ret.Resize({N});
  ret.FillArange(static_cast<typename ArrayType::Type>(from),
                 static_cast<typename ArrayType::Type>(to));
}
}  // namespace details

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

  explicit Tensor(ContainerType &container_data)
  {
    Reshape(container_data.shape());
    data_ = container_data;
  }

  static Tensor FromString(byte_array::ConstByteArray const &c);
  explicit Tensor(SizeType const &n);
  Tensor(Tensor &&other)      = default;
  Tensor(Tensor const &other) = default;
  Tensor(SizeVector const &dims);
  virtual ~Tensor() = default;

  Tensor &operator=(Tensor const &other) = default;
  Tensor &operator=(Tensor &&other) = default;

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
  std::enable_if_t<std::is_integral<S>::value, Type> &operator[](S const &i);
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
  ConstSliceType Slice(SizeVector index, SizeVector axes) const;
  TensorSlice    Slice();
  TensorSlice    Slice(SizeType index, SizeType axis = 0);
  TensorSlice    Slice(std::pair<SizeType, SizeType> start_end_index, SizeType axis = 0);
  TensorSlice    Slice(SizeVector index, SizeVector axes);

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
                                   SizeType const axis);

  void Sort();
  void Sort(memory::Range const &range);

  static Tensor Arange(Type const &from, Type const &to, Type const &delta);

  ////////////////////////////
  /// COMPARISON OPERATORS ///
  ////////////////////////////

  bool AllClose(Tensor const &o, Type const &relative_tolerance = Type(1e-5),
                Type const &absolute_tolerance = Type(1e-8)) const;
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
    TensorSlice       Slice(SizeType i, SizeType axis);
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
    return static_cast<SizeType>(index) * stride_[N] +
           UnrollComputeColIndex<N + 1>(std::forward<Indices>(indices)...);
  }

  template <SizeType N, typename FirstIndex>
  SizeType UnrollComputeColIndex(FirstIndex &&index) const
  {
    return static_cast<SizeType>(index) * stride_[N];
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
      stride.push_back(cur_stride);
      index.push_back(0);
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
    SizeType                axis_;
  };
};

/////////////////////////////////////////
/// Tensor methods: memory management ///
/////////////////////////////////////////

/**
 * This method allows Tensor instantiation from a string which is convenient for quickly writing
 * tests.
 * @tparam T Type
 * @tparam C Container
 * @param c bytearray indicating the values to fill the array with
 * @return Return Tensor with the specified values
 */
template <typename T, typename C>
Tensor<T, C> Tensor<T, C>::FromString(byte_array::ConstByteArray const &c)
{
  Tensor            ret;
  SizeType          n = 1;
  std::vector<Type> elems;
  elems.reserve(1024);
  bool failed = false;

  // Text parsing loop
  for (SizeType i = 0; i < c.size();)
  {
    SizeType last = i;
    switch (c[i])
    {
    case ';':
      if (i < c.size() - 1)
      {
        ++n;
      }
      ++i;
      break;
    case ',':
    case ' ':
    case '\n':
    case '\t':
    case '\r':
      ++i;
      break;
    default:
      if (byte_array::consumers::NumberConsumer<1, 2>(c, i) == -1)
      {
        throw std::runtime_error("invalid character used in string to set tensor");
      }
      else
      {
        std::string cur_elem((c.char_pointer() + last), static_cast<std::size_t>(i - last));
        auto        float_val = std::atof(cur_elem.c_str());
        elems.push_back(Type(float_val));
      }
      break;
    }
  }
  SizeType m = elems.size() / n;

  if ((m * n) != elems.size())
  {
    failed = true;
  }

  if (!failed)
  {
    ret.Resize({n, m});

    SizeType k = 0;
    for (SizeType i = 0; i < n; ++i)
    {
      for (SizeType j = 0; j < m; ++j)
      {
        ret(i, j) = elems[k++];
      }
    }
  }

  return ret;
}

///////////////////////////
/// Tensor Constructors ///
///////////////////////////

/**
 * Constructor builds an Tensor with n elements initialised to 0
 * @param n   number of elements in array (no shape specified, assume 1-D)
 */
template <typename T, typename C>
Tensor<T, C>::Tensor(SizeType const &n)
{
  this->Resize({n});
}

/**
 * Constructor builds an empty Tensor pre-initialiing with zeros from a vector of dimension
 * lengths
 * @param shape   vector of lengths for each dimension
 */
template <typename T, typename C>
Tensor<T, C>::Tensor(SizeVector const &dims)
{
  Resize(dims);
}

/////////////////////////////////
/// Tensor methods: iterators ///
/////////////////////////////////

template <typename T, typename C>
typename Tensor<T, C>::IteratorType Tensor<T, C>::begin()
{
  return IteratorType(data().pointer(), size(), data().size(), height(), padded_height());
}

template <typename T, typename C>
typename Tensor<T, C>::IteratorType Tensor<T, C>::end()
{
  return IteratorType(data().pointer() + data().size(), size(), data().size(), height(),
                      padded_height());
}

template <typename T, typename C>
typename Tensor<T, C>::ConstIteratorType Tensor<T, C>::begin() const
{
  return ConstIteratorType(data().pointer(), size(), data().size(), height(), padded_height());
}

template <typename T, typename C>
typename Tensor<T, C>::ConstIteratorType Tensor<T, C>::end() const
{
  return ConstIteratorType(data().pointer() + data().size(), size(), data().size(), height(),
                           padded_height());
}

template <typename T, typename C>
typename Tensor<T, C>::ConstIteratorType Tensor<T, C>::cbegin() const
{
  return ConstIteratorType(data().pointer(), size(), data().size(), height(), padded_height());
}

template <typename T, typename C>
typename Tensor<T, C>::ConstIteratorType Tensor<T, C>::cend() const
{
  return ConstIteratorType(data().pointer() + data().size(), size(), data().size(), height(),
                           padded_height());
}

///////////////////////
/// View Extraction ///
///////////////////////

/**
 * returns a view of the entire tensor
 * @tparam T Type
 * @tparam C Container
 * @return
 */
template <typename T, typename C>
typename Tensor<T, C>::ViewType Tensor<T, C>::View()
{
  assert(!shape_.empty());

  SizeType N = shape_.size() - 1;
  // padded_height can be 32 bytes on AVX2, set width to 1 to avoid zero-width tensors
  SizeType width = std::max(shape_[N] * stride_[N] / padded_height_, static_cast<SizeType>(1));
  assert(width > 0);
  return TensorView<Type, ContainerType>(data_, height(), width);
}

/**
 * returns a constant view of the entire tensor
 * @tparam T Type
 * @tparam C Container
 * @return
 */
template <typename T, typename C>
typename Tensor<T, C>::ViewType const Tensor<T, C>::View() const
{
  assert(!shape_.empty());

  SizeType N = shape_.size() - 1;
  // padded_height can be 32 bytes on AVX2, set width to 1 to avoid zero-width tensors
  SizeType width = std::max(shape_[N] * stride_[N] / padded_height_, static_cast<SizeType>(1));
  assert(width > 0);
  return TensorView<Type, ContainerType>(data_, height(), width);
}

/**
 * returns a tensor view based on the trailing dimension
 * @tparam T Type
 * @tparam C Container
 * @param index which index of the trailing dimension to view
 * @return
 */
template <typename T, typename C>
typename Tensor<T, C>::ViewType Tensor<T, C>::View(SizeType index)
{
  assert(shape_.size() >= 2);

  SizeType N                = shape_.size() - 1 - 1;
  SizeType dimension_length = (N == 0 ? padded_height_ : shape_[N]);
  SizeType volume           = dimension_length * stride_[N];
  SizeType width            = volume / padded_height_;
  SizeType offset           = volume * index;
  return TensorView<Type, ContainerType>(data_, height(), width, offset);
}

template <typename T, typename C>
typename Tensor<T, C>::ViewType const Tensor<T, C>::View(SizeType index) const
{
  assert(shape_.size() >= 2);

  SizeType N                = shape_.size() - 1 - 1;
  SizeType dimension_length = (N == 0 ? padded_height_ : shape_[N]);
  SizeType volume           = dimension_length * stride_[N];
  SizeType width            = volume / padded_height_;
  SizeType offset           = volume * index;
  return TensorView<Type, ContainerType>(data_, height(), width, offset);
}

template <typename T, typename C>
typename Tensor<T, C>::ViewType Tensor<T, C>::View(std::vector<SizeType> indices)
{
  assert(shape_.size() >= 1 + indices.size());

  SizeType N                = shape_.size() - 1 - indices.size();
  SizeType dimension_length = (N == 0 ? padded_height_ : shape_[N]);
  SizeType volume           = dimension_length * stride_[N];
  SizeType width            = volume / padded_height_;
  SizeType offset           = 0;

  for (SizeType i = 0; i < indices.size(); ++i)
  {
    SizeType g = N + i + 1;
    offset += stride_[g] * indices[i];
  }

  return TensorView<Type, ContainerType>(data_, height(), width, offset);
}

template <typename T, typename C>
typename Tensor<T, C>::ViewType const Tensor<T, C>::View(std::vector<SizeType> indices) const
{
  assert(shape_.size() >= 1 + indices.size());

  SizeType N                = shape_.size() - 1 - indices.size();
  SizeType dimension_length = (N == 0 ? padded_height_ : shape_[N]);
  SizeType volume           = dimension_length * stride_[N];
  SizeType width            = volume / padded_height_;
  SizeType offset           = 0;

  for (SizeType i = 0; i < indices.size(); ++i)
  {
    SizeType g = N + i + 1;
    offset += stride_[g] * indices[i];
  }

  return TensorView<Type, ContainerType>(data_, height(), width, offset);
}

//////////////////////////////////////////////
/// Tensor methods: assignment & accessing ///
//////////////////////////////////////////////

/**
 * Copies input data into current array
 *
 * @param[in]     x is another Tensor of which the data, size, and shape will be copied locally.
 *
 **/
template <typename T, typename C>
void Tensor<T, C>::Copy(Tensor const &x)
{
  this->data_          = x.data_.Copy();
  this->size_          = x.size_;
  this->padded_height_ = x.padded_height_;
  this->shape_         = x.shape();
  this->stride_        = x.stride();
}

/**
 * Provides an Tensor that is a copy of the current Tensor
 *
 * @return A Tensor with the same data, size, and shape of this array.
 *
 **/
template <typename T, typename C>
Tensor<T, C> Tensor<T, C>::Copy() const
{
  Tensor copy;
  copy.Copy(*this);
  return copy;
}

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
 * Assign makes a deep copy from the data in the tensorslice into this tensor
 * @tparam T Type
 * @tparam C Container
 * @param other TensorSlice of another Tensor to assign data into this
 */
template <typename T, typename C>
void Tensor<T, C>::Assign(TensorSlice const &other)
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

/**
 * Assign makes a deep copy of data from another tensor into this one
 * @tparam T Type
 * @tparam C Container
 * @param other Another Tensor to assign data from into this
 */
template <typename T, typename C>
void Tensor<T, C>::Assign(Tensor const &other)
{
  if (this->size() == other.size())
  {
    auto it1 = begin();
    auto it2 = other.begin();

    while (it1.is_valid())
    {
      *it1 = *it2;
      ++it1;
      ++it2;
    }
  }
  else
  {
    if (!(Broadcast(
            [](const T &x, const T &y, T &z) {
              FETCH_UNUSED(x);
              z = y;
            },
            *this, other, *this)))
    {
      throw std::runtime_error("arrays not broadcastable for assignment!");
    }
  }
}

/**
 * Assign makes a deep copy of data from another tensor into this one
 * @tparam T
 * @tparam C
 * @param other Another tensorview to assign data from into this
 */
template <typename T, typename C>
void Tensor<T, C>::Assign(TensorView<Type, ContainerType> const &other)
{
  auto this_view = this->View();
  this_view.Assign(other);
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
  assert(sizeof...(indices) == stride_.size());
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
  assert(sizeof...(indices) == stride_.size());
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
 * Operator for accessing data in the tensor
 * @tparam T Type
 * @tparam C Container
 * @tparam Indices parameter pack of indices
 * @param indices Indices for accessing the data
 * @return Returns the value in tensor at the location specified
 */
template <typename T, typename C>
typename Tensor<T, C>::Type Tensor<T, C>::operator()(SizeType const &index) const
{
  assert(index < this->size_);
  return operator[](index);
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
 * Assignment operator uses iterators to assign every value in the
 * const slice of a Tensor into this tensor
 * @tparam T Type
 * @tparam C Container
 * @param slice Tensor slice of another tensor object
 * @return This tensor after assignment
 */
template <typename T, typename C>
Tensor<T, C> &Tensor<T, C>::operator=(ConstSliceType const &slice)
{
  auto it1 = begin();
  auto it2 = slice.begin();
  assert(it1.size() == it2.size());
  while (it1.is_valid())
  {
    *it1 = *it2;
    ++it1;
    ++it2;
  }
  return *this;
}

/**
 * Assignment operator uses iterators to assign every value in the
 * slice of a Tensor into this tensor
 * @tparam T
 * @tparam C
 * @param slice
 * @return
 */
template <typename T, typename C>
Tensor<T, C> &Tensor<T, C>::operator=(TensorSlice const &slice)
{
  auto it1 = begin();
  auto it2 = slice.begin();
  assert(it1.size() == it2.size());
  while (it1.is_valid())
  {
    *it1 = *it2;
    ++it1;
    ++it2;
  }
  return *this;
}

/**
 * Resizes and reshapes tensor according to newly specified shape
 * @param shape the new shape to set
 * @param copy whether to copy old data to new container or not
 */
template <typename T, typename C>
bool Tensor<T, C>::Resize(SizeVector const &shape, bool copy)
{
  // if the shape is exactly the same and a copy of value is required, dont do anything
  if ((this->shape() == shape) && copy)
  {
    return true;
  }

  // a shallow copy for speedy initializion of a tensor
  Tensor   old_tensor        = *this;
  SizeType old_size          = this->size();
  SizeType new_size_unpadded = Tensor::SizeFromShape(shape);
  if (copy && (old_size == new_size_unpadded))
  {
    old_tensor = this->Copy();
  }

  SizeType new_size = Tensor::PaddedSizeFromShape(shape);
  data_             = ContainerType(new_size);
  data_.SetAllZero();
  shape_         = shape;
  size_          = new_size_unpadded;
  padded_height_ = PadValue(shape[0]);
  UpdateStrides();

  // Effectively a reshape
  if (copy && (size_ == old_size))
  {
    auto it  = begin();
    auto oit = old_tensor.begin();
    assert(it.size() == oit.size());
    while (it.is_valid())
    {
      *it = *oit;
      ++it;
      ++oit;
    }
    return true;
  }
  return false;
}

/**
 * Resizes and reshapes tensor according to newly specified shape
 * @param shape the new shape to set
 */
template <typename T, typename C>
bool Tensor<T, C>::Reshape(SizeVector const &shape)
{
  return Resize(shape, true);
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
  assert(sizeof...(args) == stride_.size() + 1);  // Plus one as last arg is value
  if (sizeof...(args) != (stride_.size() + 1))
  {
    throw std::runtime_error("too many or not enough indices given to Tensor::Set");
  }

  uint64_t index = TensorSetter<0, Args...>::IndexOf(stride_, shape_, std::forward<Args>(args)...);
  Type     value = TensorSetter<0, Args...>::ValueOf(std::forward<Args>(args)...);

  data_[index] = std::move(value);
}

/**
 * Fills entire tensor with value
 * @param value
 * @param range
 */
template <typename T, typename C>
void Tensor<T, C>::Fill(Type const &value, memory::Range const &range)
{
  VectorRegisterType val(value);

  this->data().in_parallel().Apply(range, [val](VectorRegisterType &z) { z = val; });
}

/**
 * Fill entire Tensor with value
 * @param value
 */
template <typename T, typename C>
void Tensor<T, C>::Fill(Type const &value)
{
  for (auto &x : *this)
  {
    x = value;
  }
  // TODO(?): Implement all relevant vector functions
  // VectorRegisterType val(value);
  // this->data().in_parallel().Apply([val](VectorRegisterType &z) { z = val; });
}

/**
 * Sets all elements to zero
 * @tparam T Type
 * @tparam C Container
 */
template <typename T, typename C>
void Tensor<T, C>::SetAllZero()
{
  data().SetAllZero();
}

/**
 * Sets all elements to one
 * @tparam T Type
 * @tparam C Container
 */
template <typename T, typename C>
void Tensor<T, C>::SetAllOne()
{
  auto it = this->begin();
  while (it.is_valid())
  {
    *it = Type(1);
    ++it;
  }
}

/**
 * Set all padded bytes to zero.
 *
 * This method sets the padded bytes to zero. Padded bytes are those
 * which are added to ensure that the arrays true size is a multiple
 * of the vector unit.
 */
template <typename T, typename C>
void Tensor<T, C>::SetPaddedZero()
{
  data().SetPaddedZero();
}

/**
 * returns a const ref to the underlying data
 * @tparam T
 * @tparam C
 * @return
 */
template <typename T, typename C>
typename Tensor<T, C>::ContainerType const &Tensor<T, C>::data() const
{
  return data_;
}

template <typename T, typename C>
typename Tensor<T, C>::ContainerType &Tensor<T, C>::data()
{
  return data_;
}

/**
 * Fills the current array with a range
 * @tparam Unsigned an unsigned integer type
 * @param from starting point of range
 * @param to end of range
 * @return a reference to this
 */
template <typename T, typename C>
Tensor<T, C> Tensor<T, C>::FillArange(Type const &from, Type const &to)
{
  Tensor ret;

  SizeType N     = this->size();
  auto     d     = static_cast<Type>(from);
  Type     delta = static_cast<Type>(to - from) / static_cast<Type>(N);
  for (SizeType i = 0; i < N; ++i)
  {
    this->operator[](i) = Type(d);
    d += delta;
  }
  return *this;
}

/**
 * return a tensor filled with uniform random numbers
 * @tparam T Type
 * @tparam C Container
 * @param N The new size of the tensor after filling with random
 * @return The Tensor to return
 */
template <typename T, typename C>
Tensor<T, C> Tensor<T, C>::UniformRandom(SizeType const &N)
{
  Tensor ret;
  ret.Resize({N});
  ret.FillUniformRandom();

  return ret;
}

/**
 * Returns a tensor filled with uniform random integers
 * @tparam T Type
 * @tparam C Container
 * @param N The new size after assigning value
 * @param min the minimum possible random value
 * @param max the maximum possible random value
 * @return The return Tensor filled with random values
 */
template <typename T, typename C>
Tensor<T, C> Tensor<T, C>::UniformRandomIntegers(SizeType const &N, int64_t min, int64_t max)
{
  Tensor ret;
  ret.Resize({N});
  ret.FillUniformRandomIntegers(min, max);

  return ret;
}

/**
 * Fills tensor with uniform random data
 * @tparam T Type
 * @tparam C Container
 * @return The return Tensor filled with random valuess
 */
template <typename T, typename C>
Tensor<T, C> &Tensor<T, C>::FillUniformRandom()
{
  for (SizeType i = 0; i < this->size(); ++i)
  {
    this->operator[](i) = Type(random::Random::generator.AsDouble());
  }
  return *this;
}

/**
 * Fills tensor with uniform random integers
 * @tparam T
 * @tparam C
 * @param min
 * @param max
 * @return Fills tensor with random integers
 */
template <typename T, typename C>
Tensor<T, C> &Tensor<T, C>::FillUniformRandomIntegers(int64_t min, int64_t max)
{
  assert(min <= max);

  auto diff = uint64_t(max - min);

  for (SizeType i = 0; i < this->size(); ++i)
  {
    this->operator[](i) = Type(int64_t(random::Random::generator() % diff) + min);
  }

  return *this;
}

/**
 * Method returning an Tensor of zeroes
 *
 * @param shape : a vector representing the shape of the Tensor
 * @return Tensor with all zeroes
 */
template <typename T, typename C>
Tensor<T, C> Tensor<T, C>::Zeroes(SizeVector const &shape)
{
  SizeType n = PaddedSizeFromShape(shape);
  Tensor   output{n};
  output.SetAllZero();
  output.Reshape(shape);
  return output;
}

/**
 * Method returning an Tensor of ones
 *
 * @param shape : a vector representing the shape of the Tensor
 * @return Tensor with all ones
 */
template <typename T, typename C>
Tensor<T, C> Tensor<T, C>::Ones(SizeVector const &shape)
{

  Tensor output{shape};
  output.SetAllOne();
  return output;
}

/**
 * Copmutes the single value index of a datum in tensor from a n-dim vector of indices
 * @param indices dimension indices
 * @return index in the underlying data structure
 */
template <typename T, typename C>
SizeType Tensor<T, C>::ComputeIndex(SizeVector const &indices) const
{
  return ComputeColIndex(indices);
}

////////////////////////////////////
/// Tensor methods: shape & size ///
////////////////////////////////////

/**
 * compute tensor size based on the shape
 * @tparam T
 * @tparam C
 * @param shape
 * @return
 */
template <typename T, typename C>
typename Tensor<T, C>::SizeType Tensor<T, C>::SizeFromShape(SizeVector const &shape)
{
  if (shape.empty())
  {
    return SizeType{0};
  }
  return std::accumulate(std::begin(shape), std::end(shape), SizeType(1), std::multiplies<>());
}

template <typename T, typename C>
typename Tensor<T, C>::SizeType Tensor<T, C>::PaddedSizeFromShape(SizeVector const &shape)
{
  if (shape.empty())
  {
    return SizeType{0};
  }
  return PadValue(shape[0]) *
         std::accumulate(std::begin(shape) + 1, std::end(shape), SizeType(1), std::multiplies<>());
}

/**
 * Flattens the array to 1 dimension
 * @tparam T
 * @tparam C
 */
template <typename T, typename C>
void Tensor<T, C>::Flatten()
{
  Reshape({size()});
}

/**
 * Instantiates a new tensor which is the transpose of this 2D tensor
 * @tparam T Type
 * @tparam C Container
 * @return Returns new transposed Tensor
 */
template <typename T, typename C>
Tensor<T, C> Tensor<T, C>::Transpose() const
{
  // TODO (private 867) -
  assert(shape_.size() == 2);
  SizeVector new_axes{1, 0};

  Tensor ret({shape().at(1), shape().at(0)});
  TransposeImplementation(new_axes, ret);
  return ret;
}

/**
 * Instantiates a new tensor which is the transpose of this ND tensor by the specified axes
 * @tparam T Type
 * @tparam C Container
 * @param new_axes the new order of the axes
 * @return New tensor transposed as determined by new_axes
 */
template <typename T, typename C>
Tensor<T, C> Tensor<T, C>::Transpose(SizeVector &new_axes) const
{
  assert(shape_.size() > 1);
  assert(shape_.size() == new_axes.size());

  SizeVector new_shape;
  new_shape.reserve(new_shape.size());

  for (auto &val : new_axes)
  {
    new_shape.push_back(shape_.at(val));
  }

  Tensor ret(new_shape);

  TransposeImplementation(new_axes, ret);
  return ret;
}

/**
 * Removes the leading dimension of the tensor if it has size 1
 * @tparam T Type
 * @tparam C Container
 * @return This tensor after squeezing
 */
template <typename T, typename C>
Tensor<T, C> &Tensor<T, C>::Squeeze()
{
  auto shape = shape_;

  bool     not_found = true;
  SizeType cur_dim   = shape.size() - 1;
  while (not_found)
  {
    if (shape.at(cur_dim) == static_cast<SizeType>(1))
    {
      shape.erase(shape.begin() + static_cast<int32_t>(cur_dim));
      Reshape(shape);
      not_found = false;
    }
    else
    {
      if (cur_dim == 0)
      {
        throw std::runtime_error("cannot squeeze tensor, no dimensions of size 1");
      }
      --cur_dim;
    }
  }
  return *this;
}

/**
 * Adds a leading dimension of size 1
 * @tparam T Type
 * @tparam C Container
 * @return This tensor after unsqueeze
 */
template <typename T, typename C>
Tensor<T, C> &Tensor<T, C>::Unsqueeze()
{
  auto shape = shape_;
  shape.push_back(1);

  Reshape(shape);

  return *this;
}

/**
 */

/**
 * returns the tensor's current shape
 * @return the stride of the tensor as a vector of size_type
 */
template <typename T, typename C>
typename Tensor<T, C>::SizeVector const &Tensor<T, C>::stride() const
{
  return stride_;
}

/**
 * returns the tensor's current shape
 * @return  shape_ is the shape of the tensor as a vector of size_type
 */
template <typename T, typename C>
typename Tensor<T, C>::SizeVector const &Tensor<T, C>::shape() const
{
  return shape_;
}

/**
 * returns the size of a specified dimension
 * @tparam T Type
 * @tparam C Container
 * @param n the dimension to query
 * @return SizeType value indicating the size of a dimension of the Tensor
 */
template <typename T, typename C>
typename Tensor<T, C>::SizeType const &Tensor<T, C>::shape(SizeType const &n) const
{
  return shape_[n];
}

/**
 * returns the size of the tensor
 * @tparam T Type
 * @tparam C Container
 * @return SizeType value indicating total size of Tensor
 */
template <typename T, typename C>
typename Tensor<T, C>::SizeType Tensor<T, C>::size() const
{
  return size_;
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
  assert(indices.size() == shape_.size());
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

///////////////////////////////////////
/// Tensor methods: math operations ///
///////////////////////////////////////

/**
 * adds two Tensors together and supports broadcasting
 * @param other
 * @return
 */
template <typename T, typename C>
Tensor<T, C> Tensor<T, C>::InlineAdd(Tensor const &other)
{
  Add(*this, other, *this);
  return *this;
}

/**
 * adds a scalar to every element in the array and returns the new output
 * @param scalar to add
 * @return new array output
 */
template <typename T, typename C>
Tensor<T, C> Tensor<T, C>::InlineAdd(Type const &scalar)
{
  Add(*this, scalar, *this);
  return *this;
}

/**
 * Subtract one Tensor from another and support broadcasting
 * @param other
 * @return
 */
template <typename T, typename C>
Tensor<T, C> Tensor<T, C>::InlineSubtract(Tensor const &other)
{
  Subtract(*this, other, *this);
  return *this;
}

/**
 * subtract a scalar from every element in the array and return the new output
 * @param scalar to subtract
 * @return new array output
 */
template <typename T, typename C>
Tensor<T, C> Tensor<T, C>::InlineSubtract(Type const &scalar)
{
  Subtract(*this, scalar, *this);
  return *this;
}

/**
 * Subtract one Tensor from another and support broadcasting
 * @param other
 * @return
 */
template <typename T, typename C>
Tensor<T, C> Tensor<T, C>::InlineReverseSubtract(Tensor const &other)
{
  Subtract(other, *this, *this);
  return *this;
}

/**
 * subtract every element from the scalar and return the new output
 * @param scalar to subtract
 * @return new array output
 */
template <typename T, typename C>
Tensor<T, C> Tensor<T, C>::InlineReverseSubtract(Type const &scalar)
{
  Subtract(scalar, *this, *this);
  return *this;
}

/**
 * multiply other tensor by this tensor and returns this
 * @tparam T Type
 * @tparam C Container
 * @param other other tensor
 * @return returns this tensor after multiplication
 */
template <typename T, typename C>
Tensor<T, C> Tensor<T, C>::InlineMultiply(Tensor const &other)
{
  Multiply(*this, other, *this);
  return *this;
}

/**
 * multiplies array by a scalar element wise
 * @param scalar to add
 * @return new array output
 */
template <typename T, typename C>
Tensor<T, C> Tensor<T, C>::InlineMultiply(Type const &scalar)
{
  Multiply(*this, scalar, *this);
  return *this;
}

/**
 * Divide Tensor by another Tensor from another and support broadcasting
 * @param other
 * @return
 */
template <typename T, typename C>
Tensor<T, C> Tensor<T, C>::InlineDivide(Tensor const &other)
{
  Divide(*this, other, *this);
  return *this;
}

/**
 * Divide array by a scalar elementwise
 * @param scalar to divide
 * @return new array output
 */
template <typename T, typename C>
Tensor<T, C> Tensor<T, C>::InlineDivide(Type const &scalar)
{
  Divide(*this, scalar, *this);
  return *this;
}

/**
 * Divide another Tensor by this Tensor from another and support broadcasting
 * @param other
 * @return this tensor after inline reverse divide
 */
template <typename T, typename C>
Tensor<T, C> Tensor<T, C>::InlineReverseDivide(Tensor const &other)
{
  Divide(other, *this, *this);
  return *this;
}

/**
 * Divide scalar by array elementwise
 * @param scalar to divide
 * @return new array output
 */
template <typename T, typename C>
Tensor<T, C> Tensor<T, C>::InlineReverseDivide(Type const &scalar)
{
  Divide(scalar, *this, *this);
  return *this;
}

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

template <typename T, typename C>
typename Tensor<T, C>::Type Tensor<T, C>::Sum() const
{
  Type ret{0};
  for (auto const &v : *this)
  {
    ret += v;
  }
  return ret;
}

/**
 * Calculate the Exponentials of tensor x and stores in this
 */
template <typename T, typename C>
void Tensor<T, C>::Exp(Tensor const &x)
{
  Exp(x, *this);
}

/**
 * Calculate the ApproxSoftMax of X and store in this
 */
template <typename T, typename C>
void Tensor<T, C>::ApproxSoftMax(Tensor const &x)
{
  ApproxSoftMax(x, *this);
}

/**
 * Calculates the L2Norm of the tensor (Sqrt(Sum(Square(this)))
 * @tparam T
 * @tparam C
 * @return
 */
template <typename T, typename C>
typename Tensor<T, C>::Type Tensor<T, C>::L2Norm() const
{
  return fetch::math::L2Norm(*this);
}

/**
 * Calculates half the sum of squared elements in tensor
 * @tparam T
 * @tparam C
 * @return
 */
template <typename T, typename C>
typename Tensor<T, C>::Type Tensor<T, C>::L2Loss() const
{
  return fetch::math::L2Loss(*this);
}

/**
 * Calculates the distance between the max and min values
 */
template <typename T, typename C>
typename Tensor<T, C>::Type Tensor<T, C>::PeakToPeak() const
{
  return fetch::math::PeakToPeak(*this);
}

/**
 * Divide this array by another shapeless array and store the floating point remainder in this
 * array
 * @param x
 */
template <typename T, typename C>
void Tensor<T, C>::Fmod(Tensor const &x)
{
  Resize({x.size()});
  // TODO(?): Should use iterators
  fetch::math::Fmod(data_, x.data(), data_);
}

/**
 * Divide this array by another shapeless array and store the remainder in this array with
 * quotient rounded to int
 * @param x
 */
template <typename T, typename C>
void Tensor<T, C>::Remainder(Tensor const &x)
{
  Resize({x.size()});
  fetch::math::Remainder(data_, x.data(), data_);
}

/**
 * Apply softmax to this array
 * @param x
 * @return
 */
template <typename T, typename C>
Tensor<T, C> Tensor<T, C>::Softmax(Tensor const &x)
{
  Resize({x.size()});
  assert(x.size() == this->size());
  fetch::math::Softmax(x, *this);

  return *this;
}

//////////////////////////////////////////////
/// Tensor methods: Array Order operations ///
//////////////////////////////////////////////

/**
 * toggles the tensor major order; by default Tensors are column major order
 * @tparam T
 * @tparam C
 */
template <typename T, typename C>
void Tensor<T, C>::MajorOrderFlip()
{
  // it's rather strange to invoke ColumnToRow for a 1D array, but it's technically legal (all we
  // do is changed the label)
  if (this->shape().size() > 1)
  {
    if (MajorOrder() == MAJOR_ORDER::COLUMN)
    {
      FlipMajorOrder(MAJOR_ORDER::ROW);
      major_order_ = MAJOR_ORDER::ROW;
    }
    else
    {
      FlipMajorOrder(MAJOR_ORDER::COLUMN);
      major_order_ = MAJOR_ORDER::COLUMN;
    }
  }

  if (MajorOrder() == MAJOR_ORDER::COLUMN)
  {
    major_order_ = MAJOR_ORDER::ROW;
  }
  else
  {
    major_order_ = MAJOR_ORDER::COLUMN;
  }
}

/**
 * returns the current major order of the array
 * @return
 */
template <typename T, typename C>
typename Tensor<T, C>::MAJOR_ORDER Tensor<T, C>::MajorOrder() const
{
  return major_order_;
}

//////////////////////////////
/// Tensor methods: slices ///
//////////////////////////////

template <typename T, typename C>
typename Tensor<T, C>::ConstSliceType Tensor<T, C>::Slice() const
{
  std::vector<SizeVector> range;

  for (SizeType j = 0; j < shape().size(); ++j)
  {
    range.push_back({0, shape().at(j), 1});
  }

  return ConstSliceType(*this, range, 0);
}

/**
 * Returns a Slice that is not permitted to alter the original tensor
 * @tparam T
 * @tparam C
 * @param index
 * @param axis
 * @return
 */
template <typename T, typename C>
typename Tensor<T, C>::ConstSliceType Tensor<T, C>::Slice(SizeType index, SizeType axis) const
{
  std::vector<SizeVector> range;

  for (SizeType j = 0; j < shape().size(); ++j)
  {
    if (axis == j)
    {
      range.push_back({index, index + 1, 1});
    }
    else
    {
      range.push_back({0, shape().at(j), 1});
    }
  }

  return ConstSliceType(*this, range, axis);
}

/**
 * Returns a Slice along multiple dimensions that is not permitted to alter the original tensor
 * @tparam T
 * @tparam C
 * @param indices
 * @param axes
 * @return
 */
template <typename T, typename C>
typename Tensor<T, C>::ConstSliceType Tensor<T, C>::Slice(std::vector<SizeType> indices,
                                                          std::vector<SizeType> axes) const
{
  std::vector<std::vector<SizeType>> range;

  for (SizeType j = 0; j < shape().size(); ++j)
  {
    range.push_back({0, shape().at(j), 1});
  }

  for (SizeType j = 0; j < indices.size(); ++j)
  {
    range.at(axes.at(j)).at(0) = indices.at(j);
    range.at(axes.at(j)).at(1) = indices.at(j) + 1;
    range.at(axes.at(j)).at(2) = 1;
  }

  return ConstSliceType(*this, range, axes);
}

template <typename T, typename C>
typename Tensor<T, C>::TensorSlice Tensor<T, C>::Slice()
{
  std::vector<SizeVector> range;

  for (SizeType j = 0; j < shape().size(); ++j)
  {
    range.push_back({0, shape().at(j), 1});
  }

  return TensorSlice(*this, range, 0);
}

/**
 * Returns a Slice of the tensor
 * @tparam T
 * @tparam C
 * @param index
 * @param axis
 * @return
 */
template <typename T, typename C>
typename Tensor<T, C>::TensorSlice Tensor<T, C>::Slice(SizeType index, SizeType axis)
{
  std::vector<SizeVector> range;

  for (SizeType j = 0; j < shape().size(); ++j)
  {
    if (axis == j)
    {
      range.push_back({index, index + 1, 1});
    }
    else
    {
      range.push_back({0, shape().at(j), 1});
    }
  }

  return TensorSlice(*this, range, axis);
}

/**
 * Returns a Slice Range of the tensor
 * @tparam T
 * @tparam C
 * @param index
 * @param axis
 * @return
 */
template <typename T, typename C>
typename Tensor<T, C>::TensorSlice Tensor<T, C>::Slice(
    std::pair<SizeType, SizeType> start_end_index, SizeType axis)
{
  std::vector<SizeVector> range;

  for (SizeType j = 0; j < shape().size(); ++j)
  {
    if (axis == j)
    {
      assert(start_end_index.first < start_end_index.second);
      assert(start_end_index.first >= static_cast<SizeType>(0));
      assert(start_end_index.second <= shape(j));
      range.push_back({start_end_index.first, start_end_index.second, 1});
    }
    else
    {
      range.push_back({0, shape().at(j), 1});
    }
  }

  return TensorSlice(*this, range, axis);
}

/**
 * Returns a Slice along multiple dimensions that is not permitted to alter the original tensor
 * @tparam T
 * @tparam C
 * @param indices
 * @param axes
 * @return
 */
template <typename T, typename C>
typename Tensor<T, C>::TensorSlice Tensor<T, C>::Slice(std::vector<SizeType> indices,
                                                       std::vector<SizeType> axes)
{
  std::vector<std::vector<SizeType>> range;

  for (SizeType j = 0; j < shape().size(); ++j)
  {
    range.push_back({0, shape().at(j), 1});
  }

  for (SizeType j = 0; j < indices.size(); ++j)
  {
    range.at(axes.at(j)).at(0) = indices.at(j);
    range.at(axes.at(j)).at(1) = indices.at(j) + 1;
    range.at(axes.at(j)).at(2) = 1;
  }

  return TensorSlice(*this, range, axes);
}

////////////////////////////////////////
/// Tensor methods: general utilites ///
////////////////////////////////////////

/**
 * useful for printing tensor contents
 * @return
 */
template <typename T, typename C>
std::string Tensor<T, C>::ToString() const
{
  std::stringstream ss;
  ss << std::setprecision(5) << std::fixed << std::showpos;
  if (shape_.size() == 1)
  {
    for (SizeType i(0); i < shape_[0]; ++i)
    {
      ss << At(i) << ", ";
    }
  }
  else if (shape_.size() == 2)
  {
    for (SizeType i(0); i < shape_[0]; ++i)
    {
      for (SizeType j(0); j < shape_[1]; ++j)
      {
        if (j == (shape_[1] - 1))
        {
          ss << At(i, j) << ";";
        }
        else
        {
          ss << At(i, j) << ", ";
        }
      }
    }
  }
  else
  {
    throw std::runtime_error("cannot convert > 2D tensors to string");
  }
  return ss.str();
}

/**
 * find index of value in tensor. If it's not there return max_val
 * @param val
 * @return
 */
template <typename T, typename C>
SizeType Tensor<T, C>::Find(Type val) const
{
  SizeType idx{0};
  for (auto cur_val : *this)
  {
    if (cur_val == val)
    {
      return idx;
    }
    ++idx;
  }
  return numeric_max<SizeType>();
}

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
  ret_size.push_back(tensors.size());
  ret_size.insert(ret_size.end(), tensors.front().shape().begin(), tensors.front().shape().end());
  TensorType ret(ret_size);
  for (SizeType i(0); i < tensors.size(); ++i)
  {
    ret.Slice(i).Assign(tensors[i]);
  }
  return ret;
}

/**
 * Concatenate tensors on the specified axis and return a new Tensor. The shape of the new tensor
 * will be identical to all tensors input except on the axis specified where the shape will be the
 * sum of tensor sizes at that axis
 * @param tensors
 * @param axis
 * @returnf
 */
template <typename T, typename C>
Tensor<T, C> Tensor<T, C>::Concat(std::vector<Tensor> const &tensors, SizeType const axis)
{
  // cant concatenate a single tensor
  assert(tensors.size() > 1);
  SizeVector tensor0_shape = tensors[0].shape();
  // specified axis must be within range of tensor axes
  assert(axis < tensor0_shape.size());

  // all tensors must have same shape except on the axis dimension
  // also we need to know the sum of axis dimensions
  SizeType sum_axis_size = 0;
  for (std::size_t i = 0; i < tensors.size(); ++i)
  {
    for (std::size_t j = 0; j < tensors[i].shape().size(); ++j)
    {
      if (j != axis)
      {
        assert(tensors[i].shape()[j] == tensor0_shape[j]);
      }
      else
      {
        sum_axis_size += tensors[i].shape()[j];
      }
    }
  }

  // set up the return tensor shape
  SizeVector ret_tensor_shape{tensor0_shape};
  ret_tensor_shape[axis] = sum_axis_size;
  Tensor ret{ret_tensor_shape};

  // copy the data across for each tensor
  SizeType                cur_from{0};
  SizeType                cur_to{0};
  std::vector<SizeVector> step{ret_tensor_shape.size()};
  std::vector<SizeType>   cur_step(3);

  cur_step[2] = 1;  // stepsize always 1 for now

  for (SizeType i{0}; i < tensors.size(); ++i)
  {
    cur_to += tensors[i].shape()[axis];

    // identify the relevant subview to fill
    for (SizeType j{0}; j < ret.shape().size(); ++j)
    {
      if (j == axis)
      {
        cur_step[0] = cur_from;
        cur_step[1] = cur_to;
        step[j]     = cur_step;
      }
      else
      {
        cur_step[0] = 0;
        cur_step[1] = ret.shape()[j];
        step[j]     = cur_step;
      }
    }

    // copy the data across
    TensorSliceIterator<T, C> ret_it{ret, step};
    auto                      t_it = tensors[i].cbegin();

    while (t_it.is_valid())
    {
      *ret_it = *t_it;
      ++ret_it;
      ++t_it;
    }

    cur_from = cur_to;
  }

  return ret;
}

/**
 * Splits a Tensor up into a vector of tensors (effectively an UnConcatenate function)
 * @param tensors
 * @param axis
 * @returnf
 */
template <typename T, typename C>
typename std::vector<Tensor<T, C>> Tensor<T, C>::Split(Tensor const &    tensor,
                                                       SizeVector const &concat_points,
                                                       SizeType const    axis)
{
  std::vector<Tensor> ret{concat_points.size()};

  // Move implementation to Tensor::UnConcatenate
  SizeType                cur_from{0};
  SizeType                cur_to{0};
  std::vector<SizeVector> step{tensor.shape().size()};
  std::vector<SizeType>   cur_step(3);
  cur_step[2] = 1;  // stepsize always 1 for now

  for (SizeType i{0}; i < ret.size(); ++i)
  {
    // extract the relevant portion of the error_signal
    cur_to += concat_points[i];

    // identify the relevant subview to fill
    for (SizeType j{0}; j < tensor.shape().size(); ++j)
    {
      if (j == axis)
      {
        cur_step[0] = cur_from;
        cur_step[1] = cur_to;
        step[j]     = cur_step;
      }
      else
      {
        cur_step[0] = 0;
        cur_step[1] = tensor.shape()[j];
        step[j]     = cur_step;
      }
    }

    // copy the data across
    ConstSliceIteratorType err_it{tensor, step};

    SizeVector cur_error_tensor_shape = tensor.shape();
    cur_error_tensor_shape[axis]      = concat_points[i];
    Tensor cur_error_tensor{cur_error_tensor_shape};

    TensorSliceIterator<T, C> t_it{cur_error_tensor};

    while (t_it.is_valid())
    {
      *t_it = *err_it;
      ++err_it;
      ++t_it;
    }

    cur_from = cur_to;

    // and assign it to the relevant return error tensor
    ret[i] = cur_error_tensor;
  }

  return ret;
}

/**
 * sorts the data into ascending order
 */
template <typename T, typename C>
void Tensor<T, C>::Sort()
{
  std::sort(data_.pointer(), data_.pointer() + data_.size());
}

/**
 * sorts the data into ascending order
 * @param range
 */
template <typename T, typename C>
void Tensor<T, C>::Sort(memory::Range const &range)
{
  std::sort(data_.pointer() + range.from(), data_.pointer() + range.to());
}

/**
 * returns a range over this array defined using signed integers (i.e. permitting backward ranges)
 * @tparam Signed a signed integer type
 * @param from starting point of range
 * @param to end of range
 * @param delta the increment to step through the range - may be negative
 * @return returns a shapeless array with the values in *this over the specified range
 */
template <typename T, typename C>
Tensor<T, C> Tensor<T, C>::Arange(T const &from, T const &to, T const &delta)
{
  assert(delta != 0);
  assert(((from < to) && delta > 0) || ((from > to) && delta < 0));
  Tensor ret;
  details::ArangeImplementation(from, to, delta, ret);
  return ret;
}

//////////////////////////////////
/// Tensor methods: comparison ///
//////////////////////////////////

template <typename T, typename C>
bool Tensor<T, C>::AllClose(Tensor const &o, Type const &relative_tolerance,
                            Type const &absolute_tolerance) const
{
  // Only enforcing number of elements
  // we allow for different shapes as long as element are in same order
  assert(o.size() == this->size());
  auto it1  = this->cbegin();
  auto eit1 = this->cend();
  auto it2  = o.cbegin();

  while (it1 != eit1)
  {
    T e1 = *it1;
    T e2 = *it2;
    ++it1;
    ++it2;

    T abs_e1 = e1;
    fetch::math::Abs(abs_e1, abs_e1);
    T abs_e2 = e2;
    fetch::math::Abs(abs_e2, abs_e2);
    T abs_diff = e1 - e2;
    fetch::math::Abs(abs_diff, abs_diff);
    T tolerance = std::max(absolute_tolerance, std::max(abs_e1, abs_e2) * relative_tolerance);
    if (abs_diff > tolerance)
    {
      return false;
    }
  }
  return true;
}

/**
 * Equality operator
 * This method is sensitive to height and width
 * @param other  the array which this instance is compared against
 * @return
 */
template <typename T, typename C>
bool Tensor<T, C>::operator==(Tensor<T, C> const &other) const
{
  if (shape() != other.shape())
  {
    return false;
  }
  if (size() != other.size())
  {
    return false;
  }

  bool ret      = true;
  auto it       = cbegin();
  auto other_it = other.cbegin();
  while (ret && it.is_valid())
  {
    ret &= (*it == *other_it);
    ++it;
    ++other_it;
  }

  return ret;
}

/**
 * Not-equal operator
 * This method is sensitive to height and width
 * @param other the array which this instance is compared against
 * @return
 */
template <typename T, typename C>
bool Tensor<T, C>::operator!=(Tensor<T, C> const &other) const
{
  return !(this->operator==(other));
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
    assert(SizeType(index) < shape[N]);
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
    new_axes.push_back(this->axis_);
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
  new_axes.push_back(axis);

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
    shape.emplace_back(this->range_[i][1] - this->range_[i][0] / this->range_[i][2]);
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
    new_axes.push_back(axis_);
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
  new_axes.push_back(axis);

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
