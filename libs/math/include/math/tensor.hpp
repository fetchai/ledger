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

#include "core/assert.hpp"
#include "core/byte_array/const_byte_array.hpp"
#include "core/byte_array/consumers.hpp"
#include "core/macros.hpp"
#include "core/random.hpp"

#include "vectorise/memory/array.hpp"

#include "math/base_types.hpp"
#include "math/matrix_operations.hpp"
#include "math/meta/math_type_traits.hpp"
#include "math/ml/activation_functions/softmax.hpp"
#include "math/ml/loss_functions/l2_loss.hpp"
#include "math/ml/loss_functions/l2_norm.hpp"
#include "math/standard_functions/abs.hpp"
#include "math/standard_functions/exp.hpp"
#include "math/standard_functions/fmod.hpp"
#include "math/standard_functions/remainder.hpp"
#include "math/tensor_broadcast.hpp"
#include "math/tensor_iterator.hpp"

#include <iostream>
#include <memory>
#include <numeric>
#include <random>
#include <utility>

namespace fetch {
namespace math {

namespace details {
template <typename DataType, typename ArrayType>
static void ArangeImplementation(DataType const &from, DataType const &to, DataType const &delta,
                                 ArrayType &ret)
{
  SizeType N = SizeType((to - from) / delta);
  ret.LazyResize(N);
  ret.SetPaddedZero();
  ret.FillArange(from, to);
}
}  // namespace details

template <typename T, typename C = memory::SharedArray<T>>
class Tensor
{
public:
  using Type                       = T;
  using ContainerType              = C;
  using VectorSliceType            = typename ContainerType::VectorSliceType;
  using VectorRegisterType         = typename ContainerType::VectorRegisterType;
  using VectorRegisterIteratorType = typename ContainerType::VectorRegisterIteratorType;
  using SelfType                   = Tensor<T, C>;
  using IteratorType               = TensorIterator<T, typename SelfType::ContainerType>;
  using ConstIteratorType          = ConstTensorIterator<T, typename SelfType::ContainerType>;
  using SizeType                   = fetch::math::SizeType;
  using SizeVector                 = fetch::math::SizeVector;

  static constexpr char const *LOGGING_NAME = "Tensor";

private:
  template <typename STensor>
  class TensorSliceImplementation;
  template <SizeType N, typename TSType, typename... Args>
  struct TensorSetter;
  template <SizeType N, typename TSType>
  struct TensorSetter<N, TSType>;

public:
  using ConstSliceType = TensorSliceImplementation<SelfType const>;

  class TensorSlice;

  using SliceType = TensorSlice;

  enum MAJOR_ORDER
  {
    COLUMN,
    ROW
  };

  Tensor()
    : data_()
    , size_(0)
  {}

  static Tensor FromString(byte_array::ConstByteArray const &c);
  explicit Tensor(SizeType const &n);
  Tensor(Tensor &&other)      = default;
  Tensor(Tensor const &other) = default;
  Tensor(SizeVector const &dims);
  virtual ~Tensor()
  {}

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

  static SelfType Zeroes(SizeVector const &shape);
  static SelfType Ones(SizeVector const &shape);

  void     Copy(SelfType const &x);
  SelfType Copy() const;
  template <typename G>
  void Assign(TensorSliceImplementation<G> const &other);
  void Assign(TensorSlice const &other);
  void Assign(SelfType const &other);

  template <typename... Indices>
  Type &At(Indices... indices);
  template <typename... Indices>
  Type At(Indices... indices) const;

  template <typename... Indices>
  Type operator()(Indices... indices) const;
  template <typename... Indices>
  Type &operator()(Indices... indices);

  Type operator()(SizeType const &index) const;
  template <typename S>
  typename std::enable_if<std::is_integral<S>::value, Type>::type &operator[](S const &i);
  template <typename S>
  typename std::enable_if<std::is_integral<S>::value, Type>::type const &operator[](
      S const &i) const;

  Tensor &operator=(ConstSliceType const &slice);
  Tensor &operator=(TensorSlice const &slice);

  template <typename... Args>
  void Set(Args... args);

  void Fill(Type const &value, memory::Range const &range);
  void Fill(Type const &value, memory::TrivialRange const &range);
  void Fill(Type const &value);
  void SetAllZero();
  void SetAllOne();
  void SetPaddedZero();

  ////////////////////
  /// SHAPE & SIZE ///
  ////////////////////

  static SizeType SizeFromShape(SizeVector const &shape);

  void              Flatten();
  SelfType          Transpose() const;  // TODO (private 867)
  SelfType          Transpose(SizeVector &new_axes) const;
  SelfType &        Squeeze();
  SelfType &        Unsqueeze();
  void              ResizeFromShape(SizeVector const &shape);
  void              LazyReshape(SizeVector const &shape);
  bool              CanReshape(SizeVector const &shape);
  void              Reshape(SizeVector const &shape);
  SizeVector const &shape() const;
  SizeType const &  shape(SizeType const &n) const;

  ///////////////////////
  /// MATH OPERATIONS ///
  ///////////////////////

  SelfType InlineAdd(Tensor const &other);
  SelfType InlineAdd(Type const &scalar);
  SelfType InlineSubtract(Tensor const &other);
  SelfType InlineSubtract(Type const &scalar);
  SelfType InlineReverseSubtract(Tensor const &other);
  SelfType InlineReverseSubtract(Type const &scalar);
  SelfType InlineMultiply(Tensor const &other);
  SelfType InlineMultiply(Type const &scalar);
  SelfType InlineDivide(Tensor const &other);
  SelfType InlineDivide(Type const &scalar);
  SelfType InlineReverseDivide(Tensor const &other);
  SelfType InlineReverseDivide(Type const &scalar);

  template <typename OtherType>
  SelfType operator+(OtherType const &other);

  template <typename OtherType>
  SelfType operator+=(OtherType const &other);

  template <typename OtherType>
  SelfType operator-(OtherType const &other);

  template <typename OtherType>
  SelfType operator-=(OtherType const &other);

  template <typename OtherType>
  SelfType operator*(OtherType const &other);

  template <typename OtherType>
  SelfType operator*=(OtherType const &other);

  template <typename OtherType>
  SelfType operator/(OtherType const &other);

  template <typename OtherType>
  SelfType operator/=(OtherType const &other);

  SelfType &DotTranspose(SelfType const &A, SelfType const &B, Type alpha = 1.0, Type beta = 0.0);
  SelfType &TransposeDot(SelfType const &A, SelfType const &B, Type alpha = 1.0, Type beta = 0.0);
  Type      Sum() const;

  void Exp(SelfType const &x);
  void ApproxSoftMax(SelfType const &x);
  Type L2Norm() const;
  Type L2Loss() const;

  /////////////
  /// Order ///
  /////////////

  void        MajorOrderFlip();
  MAJOR_ORDER MajorOrder() const;

  ////////////////////////
  /// Numpy Operations ///
  ////////////////////////

  void CopyFromNumpy(T *ptr, SizeVector &shape, SizeVector & /*stride*/, SizeVector & /*index*/);
  void CopyToNumpy(T *ptr, SizeVector &shape, SizeVector &stride, SizeVector &index);

  //////////////
  /// Slices ///
  //////////////

  ConstSliceType Slice(SizeType i, SizeType axis = 0) const;
  TensorSlice    Slice(SizeType i, SizeType axis = 0);

  /////////////////
  /// general utilities
  //////////////

  std::string ToString() const;
  SizeType    Find(Type val) const;
  template <typename TensorType>
  static SelfType Stack(std::vector<TensorType> const &tensors);
  void            Sort();
  void            Sort(memory::TrivialRange const &range);

  /**
   * returns a range over this array defined using unsigned integers (only forward ranges)
   * @tparam Unsigned an unsigned integer type
   * @param from starting point of range
   * @param to end of range
   * @param delta the increment to step through the range
   * @return returns a shapeless array with the values in *this over the specified range
   */
  template <typename Unsigned>
  static fetch::meta::IfIsUnsignedInteger<Unsigned, SelfType> Arange(Unsigned const &from,
                                                                     Unsigned const &to,
                                                                     Unsigned const &delta)
  {
    assert(delta != 0);
    assert(from < to);
    SelfType ret;
    details::ArangeImplementation(from, to, delta, ret);
    return ret;
  }

  /**
   * returns a range over this array defined using signed integers (i.e. permitting backward ranges)
   * @tparam Signed a signed integer type
   * @param from starting point of range
   * @param to end of range
   * @param delta the increment to step through the range - may be negative
   * @return returns a shapeless array with the values in *this over the specified range
   */
  template <typename Signed>
  static fetch::meta::IfIsSignedInteger<Signed, SelfType> Arange(Signed const &from,
                                                                 Signed const &to,
                                                                 Signed const &delta)
  {
    assert(delta != 0);
    assert(((from < to) && delta > 0) || ((from > to) && delta < 0));
    SelfType ret;
    details::ArangeImplementation(from, to, delta, ret);
    return ret;
  }

  /**
   * Fills the current array with a range
   * @tparam Unsigned an unsigned integer type
   * @param from starting point of range
   * @param to end of range
   * @return a reference to this
   */
  template <typename DataType>
  fetch::meta::IfIsInteger<DataType, SelfType> FillArange(DataType const &from, DataType const &to)
  {
    SelfType ret;

    SizeType N     = this->size();
    Type     d     = static_cast<Type>(from);
    Type     delta = static_cast<Type>(to - from) / static_cast<Type>(N);
    for (SizeType i = 0; i < N; ++i)
    {
      this->data()[i] = Type(d);
      d += delta;
    }
    return *this;
  }

  static SelfType UniformRandom(SizeType const &N)
  {
    SelfType ret;
    ret.LazyResize(N);
    ret.SetPaddedZero();
    ret.FillUniformRandom();

    return ret;
  }

  static SelfType UniformRandomIntegers(SizeType const &N, int64_t const &min, int64_t const &max)
  {
    SelfType ret;
    ret.LazyResize(N);
    ret.SetPaddedZero();
    ret.FillUniformRandomIntegers(min, max);

    return ret;
  }

  SelfType &FillUniformRandom()
  {
    for (SizeType i = 0; i < this->size(); ++i)
    {
      this->data()[i] = Type(random::Random::generator.AsDouble());
    }
    return *this;
  }

  SelfType &FillUniformRandomIntegers(int64_t const &min, int64_t const &max)
  {
    assert(min <= max);

    uint64_t diff = uint64_t(max - min);

    for (SizeType i = 0; i < this->size(); ++i)
    {
      this->data()[i] = Type(int64_t(random::Random::generator() % diff) + min);
    }

    return *this;
  }

  bool LazyReserve(SizeType const &n)
  {
    if (data_.size() < n)
    {
      data_ = ContainerType(n);
      return true;
    }
    return false;
  }

  void Reserve(SizeType const &n)
  {
    ContainerType old_data = data_;

    if (LazyReserve(n))
    {
      SizeType ns = std::min(old_data.size(), n);
      memcpy(data_.pointer(), old_data.pointer(), ns);
      data_.SetZeroAfter(ns);
    }
  }

  void ReplaceData(SizeType const &n, ContainerType const &data)
  {
    assert(n <= data.size());
    data_ = data;
    size_ = n;
  }

  template <typename S>
  typename std::enable_if<std::is_integral<S>::value, void>::type LazyResize(S const &n)
  {
    LazyReserve(n);
    size_ = n;
    data_.SetZeroAfter(n);
  }

  template <typename S>
  typename std::enable_if<std::is_integral<S>::value, void>::type Resize(S const &n)
  {
    SizeType oldsize = size_;
    LazyResize(n);
    data_.SetZeroAfter(oldsize);
  }

  // TODO(private 858): Vectorize and deduce D from parent
  template <typename S, typename D = memory::SharedArray<S>>
  void As(Tensor<S, D> &ret) const
  {
    ret.LazyResize(size_);
    auto this_it = cbegin();
    auto ret_it  = begin();

    while (this_it.is_valid())
    {
      *ret_it = *this_it;
      ++ret_it;
      ++this_it;
    }
  }

  template <typename S>
  fetch::meta::IfIsUnsignedInteger<S, Type> Get(S const &indices) const
  {
    return data_[indices];
  }
  //  T Get(SizeType const &idx) { return data_[idx]; } const

  ContainerType const &data() const
  {
    return data_;
  }
  ContainerType &data()
  {
    return data_;
  }
  SizeType size() const
  {
    return size_;
  }

  ////////////////////////////
  /// COMPARISON OPERATORS ///
  ////////////////////////////

  bool AllClose(SelfType const &o, Type const &relative_tolerance = Type(1e-5),
                Type const &absolute_tolerance = Type(1e-8)) const
  {
    // Only enforcing number of elements
    // we allow for different shapes as long as element are in same order
    ASSERT(o.size() == this->size());
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
        std::cout << "AllClose - " << e1 << " != " << e2 << std::endl;
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
  bool operator==(Tensor const &other) const
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
  bool operator!=(Tensor const &other) const
  {
    return !(this->operator==(other));
  }

  Type PeakToPeak() const
  {
    return fetch::math::PeakToPeak(*this);
  }

  /**
   * Divide this array by another shapeless array and store the floating point remainder in this
   * array
   * @param x
   */
  void Fmod(SelfType const &x)
  {
    LazyResize(x.size());
    fetch::math::Fmod(data_, x.data(), data_);
  }

  /**
   * Divide this array by another shapeless array and store the remainder in this array with
   * quotient rounded to int
   * @param x
   */
  void Remainder(SelfType const &x)
  {
    LazyResize(x.size());
    fetch::math::Remainder(data_, x.data(), data_);
  }

  /**
   * Apply softmax to this array
   * @param x
   * @return
   */
  SelfType Softmax(SelfType const &x)
  {
    LazyResize(x.size());

    assert(x.size() == this->size());
    fetch::math::Softmax(x, *this);

    return *this;
  }

  /**
   * TensorSlice class contains the non-const methods of tensor slicing
   */
  class TensorSlice : public TensorSliceImplementation<Tensor>
  {
  public:
    using Type = T;
    using TensorSliceImplementation<Tensor>::TensorSliceImplementation;
    using TensorSliceImplementation<Tensor>::begin;
    using TensorSliceImplementation<Tensor>::end;

    IteratorType begin();
    IteratorType end();
    template <typename G>
    void Assign(TensorSliceImplementation<G> const &other);
    void Assign(Tensor const &other);
    void Fill(Type t);
  };

  ////////////////////////////////
  /// Serialization operations ///
  ////////////////////////////////

  template <typename S>
  friend void Serialize(S &serializer, SelfType const &t)
  {
    serializer << t.size_;
    serializer << t.shape_;
    // TODO (private 870)
    for (std::size_t i = 0; i < t.size(); ++i)
    {
      serializer << t.data()[i];
    }
  }

  template <typename S>
  friend void Deserialize(S &serializer, SelfType &t)
  {
    SizeType   size;
    SizeVector shape;
    serializer >> size;
    serializer >> shape;

    t.Resize(size);
    t.Reshape(shape);

    for (std::size_t i = 0; i < t.size(); ++i)
    {
      serializer >> t.data()[i];
    }
  }

private:
  ContainerType data_;
  SizeType      size_ = 0;
  SizeVector    shape_;
  SizeVector    stride_;

  MAJOR_ORDER major_order_ = COLUMN;

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
    SizeType base   = 1;

    for (SizeType i = 0; i < n_dims; ++i)
    {
      stride_[i] = base;
      base *= shape_[i];
    }

    // TODO (private 870): Reverse order if row major.
  }

  // TODO(private 871): replace with strides
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

  SizeType ComputeColIndex(SizeVector const &indices) const
  {
    SizeType index  = 0;
    SizeType n_dims = indices.size();
    SizeType base   = 1;

    // loop through all dimensions
    for (SizeType i = 0; i < n_dims; ++i)
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
    SelfType new_array{this->shape()};

    SizeVector stride;
    SizeVector index;

    SizeType cur_stride = Product(this->shape());

    for (SizeType i = 0; i < new_array.shape().size(); ++i)
    {
      cur_stride /= this->shape()[i];
      stride.push_back(cur_stride);
      index.push_back(0);
    }

    SizeType     total_size = SelfType::SizeFromShape(new_array.shape());
    IteratorType it_this(*this);

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

  void TransposeImplementation(SizeVector &new_axes, SelfType &ret) const
  {
    auto it     = this->begin();
    auto eit    = this->end();
    auto ret_it = ret.end();
    ret_it.Transpose(new_axes);

    while (it != eit)
    {
      *ret_it = *it;
      ++it;
      ++ret_it;
    }
  }

  /**
   * The TensorSlice is a convenience method for efficiently manipulating
   * SubTensors (e.g. such as a 1D Slice). It is built on top of TensorIterator
   * @tparam STensor
   */
  template <typename STensor>
  class TensorSliceImplementation
  {
  public:
    using Type = typename STensor::Type;

    TensorSliceImplementation<STensor>(STensor &t, std::vector<std::vector<SizeType>> range,
                                       SizeType axis = 0)
      : tensor_{t}
      , range_{std::move(range)}
      , axis_{std::move(axis)}
    {}

    SelfType          Copy() const;
    ConstIteratorType begin() const;
    ConstIteratorType end() const;
    STensor &         Tensor();
    SizeType          size() const;
    SizeVector        shape() const;

  protected:
    STensor &                          tensor_;
    std::vector<std::vector<SizeType>> range_;
    SizeType                           axis_;
  };
};

template <typename T, typename C>
Tensor<T, C> Tensor<T, C>::FromString(byte_array::ConstByteArray const &c)
{
  Tensor            ret;
  SizeType          n = 1;
  std::vector<Type> elems;
  elems.reserve(1024);
  bool failed = false;

  for (SizeType i = 0; i < c.size();)
  {
    SizeType last = i;
    switch (c[i])
    {
    case ';':
      ++n;
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
        failed = true;
      }
      else
      {
        elems.push_back(Type(atof(c.char_pointer() + last)));
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
    ret.ResizeFromShape({n, m});
    ret.SetAllZero();

    SizeType k = 0;
    for (SizeType i = 0; i < n; ++i)
    {
      for (SizeType j = 0; j < m; ++j)
      {
        ret.Set(i, j, elems[k++]);
      }
    }
  }

  return ret;
}

///////////////////////////
/// Tensor Constructors ///
///////////////////////////

/**
 * Constructor builds an Tensor with n elements initialized to 0
 * @param n   number of elements in array (no shape specified, assume 1-D)
 */
template <typename T, typename C>
Tensor<T, C>::Tensor(SizeType const &n)
  : data_(n)
  , size_(n)
{
  assert(this->size() == n);
  this->LazyReshape({n});
  Type zero{0};
  for (SizeType idx = 0; idx < this->size(); ++idx)
  {
    operator[](idx) = zero;
  }
}

/**
 * Constructor builds an empty Tensor pre-initialiing with zeros from a vector of dimension
 * lengths
 * @param shape   vector of lengths for each dimension
 */
template <typename T, typename C>
Tensor<T, C>::Tensor(SizeVector const &dims)
{
  ResizeFromShape(dims);
  this->SetAllZero();
}

/////////////////////////////////
/// Tensor methods: iterators ///
/////////////////////////////////

template <typename T, typename C>
typename Tensor<T, C>::IteratorType Tensor<T, C>::begin()
{
  return IteratorType(*this);
}

template <typename T, typename C>
typename Tensor<T, C>::IteratorType Tensor<T, C>::end()
{
  return IteratorType::EndIterator(*this);
}

template <typename T, typename C>
typename Tensor<T, C>::ConstIteratorType Tensor<T, C>::begin() const
{
  return ConstIteratorType(*this);
}

template <typename T, typename C>
typename Tensor<T, C>::ConstIteratorType Tensor<T, C>::end() const
{
  return ConstIteratorType::EndIterator(*this);
}

template <typename T, typename C>
typename Tensor<T, C>::ConstIteratorType Tensor<T, C>::cbegin() const
{
  return ConstIteratorType(*this);
}

template <typename T, typename C>
typename Tensor<T, C>::ConstIteratorType Tensor<T, C>::cend() const
{
  return ConstIteratorType::EndIterator(*this);
}

//////////////////////////////////////////////
/// Tensor methods: assignment & accessing ///
//////////////////////////////////////////////

/**
 * Method returning an Tensor of zeroes
 *
 * @param shape : a vector representing the shape of the Tensor
 * @return Tensor with all zeroes
 */
template <typename T, typename C>
Tensor<T, C> Tensor<T, C>::Zeroes(SizeVector const &shape)
{
  SizeType n = SizeFromShape(shape);
  SelfType output{n};
  output.SetAllZero();
  output.LazyReshape(shape);
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
  SizeType n =
      std::accumulate(std::begin(shape), std::end(shape), SizeType(1), std::multiplies<SizeType>());
  SelfType output{n};
  output.SetAllOne();
  output.LazyReshape(shape);
  return output;
}

/**
 * Copies input data into current array
 *
 * @param[in]     x is another Tensor of which the data, size, and shape will be copied locally.
 *
 **/
template <typename T, typename C>
void Tensor<T, C>::Copy(SelfType const &x)
{
  this->data_ = x.data_.Copy();
  this->size_ = x.size_;
  this->LazyReshape(x.shape());
}

/**
 * Provides an Tensor that is a copy of the current Tensor
 *
 * @return       copy is a Tensor with the same data, size, and shape of this array.
 *
 **/
template <typename T, typename C>
Tensor<T, C> Tensor<T, C>::Copy() const
{
  SelfType copy;
  copy.data_ = this->data_.Copy();
  copy.size_ = this->size_;
  copy.LazyReshape(this->shape());
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
  auto it2 = other.begin();
  ASSERT(it1.size() == it2.size());
  while (it1.is_valid())
  {
    *it1 = *it2;
    ++it1;
    ++it2;
  }
}

/**
 *
 * @param other
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
 * assign makes a deep copy of data from another tensor into this one
 * @param other
 */
template <typename T, typename C>
void Tensor<T, C>::Assign(SelfType const &other)
{
  auto it1 = begin();
  auto it2 = other.begin();
  ASSERT(it1.size() == it2.size());
  while (it1.is_valid())
  {
    *it1 = *it2;
    ++it1;
    ++it2;
  }
}

/**
 *
 * @tparam Indices
 * @param indices
 * @return
 */
template <typename T, typename C>
template <typename... Indices>
typename Tensor<T, C>::Type &Tensor<T, C>::At(Indices... indices)
{
  ASSERT(sizeof...(indices) == stride_.size());
  return this->data()[UnrollComputeColIndex<0>(std::forward<Indices>(indices)...)];
}

/**
 *
 * @tparam Indices
 * @param indices
 * @return
 */
template <typename T, typename C>
template <typename... Indices>
typename Tensor<T, C>::Type Tensor<T, C>::At(Indices... indices) const
{
  ASSERT(sizeof...(indices) == stride_.size());
  SizeType N = UnrollComputeColIndex<0>(std::forward<Indices>(indices)...);
  return this->data()[std::move(N)];
}

/**
 * Operator for accessing data in the array
 *
 * @param[in]     indices specifies the data points to access.
 * @return        the accessed data.
 *
 **/
template <typename T, typename C>
template <typename... Indices>
typename Tensor<T, C>::Type Tensor<T, C>::operator()(Indices... indices) const
{
  return At(std::forward<Indices>(indices)...);
}

template <typename T, typename C>
template <typename... Indices>
typename Tensor<T, C>::Type &Tensor<T, C>::operator()(Indices... indices)
{
  return At(std::forward<Indices>(indices)...);
}

template <typename T, typename C>
typename Tensor<T, C>::Type Tensor<T, C>::operator()(SizeType const &index) const
{
  assert(index < this->size_);
  return operator[](index);
}

/**
 * One-dimensional reference index operator.
 *
 * This operator acts as a one-dimensional array accessor that is
 * meant for non-constant object instances.
 *
 * @tparam S an integral type
 * @param i the index to access
 * @return
 */
template <typename T, typename C>
template <typename S>
typename std::enable_if<std::is_integral<S>::value, typename Tensor<T, C>::Type>::type
    &Tensor<T, C>::operator[](S const &i)
{
  return data_[i];
}

/**
 * One-dimensional reference index operator.
 *
 * This operator acts as a one-dimensional array accessor that is
 * meant for non-constant object instances.
 *
 * @tparam S an integral type
 * @param i the index to access
 * @return
 */
template <typename T, typename C>
template <typename S>
typename std::enable_if<std::is_integral<S>::value, typename Tensor<T, C>::Type>::type const
    &Tensor<T, C>::operator[](S const &i) const
{
  return data_[i];
}

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
 * Set operator takes variable number of indices followed by one value.
 *
 * @tparam T
 * @tparam C
 * @tparam Args
 * @param args
 */
template <typename T, typename C>
template <typename... Args>
void Tensor<T, C>::Set(Args... args)
{
  ASSERT(sizeof...(args) == stride_.size() + 1);  // Plus one as last arg is value

  uint64_t index = TensorSetter<0, Args...>::IndexOf(stride_, shape_, std::forward<Args>(args)...);
  Type     value = TensorSetter<0, Args...>::ValueOf(std::forward<Args>(args)...);

  data_[std::move(index)] = std::move(value);
}

/**
 * Fill tensor with specified value over pre-specified range
 * @tparam T
 * @tparam C
 * @param value
 * @param range
 */
template <typename T, typename C>
void Tensor<T, C>::Fill(Type const &value, memory::Range const &range)
{

  if (range.is_undefined())
  {
    Fill(value);
  }
  else if (range.is_trivial())
  {
    Fill(value, range.ToTrivialRange(this->size()));
  }
  else
  {
    TODO_FAIL("Support for general range is not implmenented yet");
  }
}

/**
 * Fills entire tensor with value
 * @param value
 * @param range
 */
template <typename T, typename C>
void Tensor<T, C>::Fill(Type const &value, memory::TrivialRange const &range)
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
  VectorRegisterType val(value);

  this->data().in_parallel().Apply([val](VectorRegisterType &z) { z = val; });
}

/**
 * Set all elements to zero.
 * This method will initialise all memory with zero.
 */
template <typename T, typename C>
void Tensor<T, C>::SetAllZero()
{
  data().SetAllZero();
}

/**
 * Sets all value in tensor to 1
 */
template <typename T, typename C>
void Tensor<T, C>::SetAllOne()
{
  auto it = this->begin();
  while (it.is_valid())
  {
    *it = 1;
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
  if (shape.size() == 0)
  {
    return SizeType{0};
  }
  return std::accumulate(std::begin(shape), std::end(shape), SizeType(1), std::multiplies<>());
}

/**
 * Flattens the array to 1 dimension
 * @tparam T
 * @tparam C
 */
template <typename T, typename C>
void Tensor<T, C>::Flatten()
{
  shape_.clear();
  shape_.push_back(size_);
  UpdateStrides();
}

/**
 *
 * @tparam T
 * @tparam C
 * @return
 */
template <typename T, typename C>
typename Tensor<T, C>::SelfType Tensor<T, C>::Transpose() const
{
  // TODO (private 867) -
  ASSERT(shape_.size() == 2);
  SizeVector new_axes{1, 0};

  SelfType ret({shape().at(1), shape().at(0)});
  TransposeImplementation(new_axes, ret);
  return ret;
}

/**
 *
 * @tparam T
 * @tparam C
 * @param new_axes
 * @return
 */
template <typename T, typename C>
typename Tensor<T, C>::SelfType Tensor<T, C>::Transpose(SizeVector &new_axes) const
{
  ASSERT(shape_.size() > 1);
  ASSERT(shape_.size() == new_axes.size());

  SelfType ret(shape());
  TransposeImplementation(new_axes, ret);
  return ret;
}

/**
 * Removes the leading dimension if it has size 1
 */
template <typename T, typename C>
typename Tensor<T, C>::SelfType &Tensor<T, C>::Squeeze()
{
  ASSERT(shape_.at(0) == 1);
  shape_.erase(shape_.begin());
  UpdateStrides();
  return *this;
}

/**
 * Adds a leading dimension of size 1
 * @tparam T
 * @tparam C
 * @return
 */
template <typename T, typename C>
typename Tensor<T, C>::SelfType &Tensor<T, C>::Unsqueeze()
{
  shape_.insert(shape_.begin(), 1);
  UpdateStrides();
  return *this;
}

/**
 *
 * @tparam T
 * @tparam C
 * @param shape
 */
template <typename T, typename C>
void Tensor<T, C>::ResizeFromShape(SizeVector const &shape)
{
  Resize(SelfType::SizeFromShape(shape));
  Reshape(shape);
}

/**
 * Directly copies shape variable without checking anything
 *
 * @param[in]     shape specifies the new shape.
 *
 **/
template <typename T, typename C>
void Tensor<T, C>::LazyReshape(SizeVector const &shape)
{
  shape_ = shape;
  UpdateStrides();
}

/**
 * Tests if it is possible to reshape the array to a newly proposed shape
 *
 * @param[in]     shape specified for the new array as a vector ot size_t.
 * @return        success is a bool indicating where the proposed shape is acceptable.
 *
 **/
template <typename T, typename C>
bool Tensor<T, C>::CanReshape(SizeVector const &shape)
{
  if ((shape.size() == 0) && (size() == 0))
  {
    return true;
  }
  else
  {
    SizeType total = 1;
    for (auto const &s : shape)
    {
      total *= s;
    }
    bool success                      = false;
    (total == this->size()) ? success = true : success = false;
    return success;
  }
  return false;
}

/**
 * Reshapes after checking the total size is the same
 * @param shape  specified for the new array as a vector of size type.
 */
template <typename T, typename C>
void Tensor<T, C>::Reshape(SizeVector const &shape)
{
  ASSERT(CanReshape(shape));

  shape_.clear();
  shape_.reserve(shape.size());
  for (auto const &s : shape)
  {
    shape_.push_back(s);
  }
  UpdateStrides();
  size_ = SelfType::SizeFromShape(shape);
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
 * @param n
 * @return
 */
template <typename T, typename C>
typename Tensor<T, C>::SizeType const &Tensor<T, C>::shape(SizeType const &n) const
{
  return shape_[n];
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
typename Tensor<T, C>::SelfType Tensor<T, C>::InlineAdd(Tensor const &other)
{
  if (other.shape() == shape_)
  {
    Add(*this, other, *this);
  }
  else
  {
    SelfType self_copy  = this->Copy();
    SelfType other_copy = other.Copy();
    if (!(Broadcast([](T x, T y) { return x + y; }, self_copy, other_copy, *this)))
    {
      throw std::runtime_error("arrays not broadcastable!");
    }
  }
  return *this;
}

/**
 * adds a scalar to every element in the array and returns the new output
 * @param scalar to add
 * @return new array output
 */
template <typename T, typename C>
typename Tensor<T, C>::SelfType Tensor<T, C>::InlineAdd(Type const &scalar)
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
typename Tensor<T, C>::SelfType Tensor<T, C>::InlineSubtract(Tensor const &other)
{
  if (other.shape() == shape_)
  {
    Subtract(*this, other, *this);
  }
  else
  {
    SelfType self_copy  = this->Copy();
    SelfType other_copy = other.Copy();
    if (!(Broadcast([](T x, T y) { return x - y; }, self_copy, other_copy, *this)))
    {
      throw std::runtime_error("arrays not broadcastable!");
    }
  }
  return *this;
}

/**
 * subtract a scalar from every element in the array and return the new output
 * @param scalar to subtract
 * @return new array output
 */
template <typename T, typename C>
typename Tensor<T, C>::SelfType Tensor<T, C>::InlineSubtract(Type const &scalar)
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
typename Tensor<T, C>::SelfType Tensor<T, C>::InlineReverseSubtract(Tensor const &other)
{
  if (other.shape() == shape_)
  {
    Subtract(other, *this, *this);
  }
  else
  {
    SelfType self_copy  = this->Copy();
    SelfType other_copy = other.Copy();
    if (!(Broadcast([](T x, T y) { return x - y; }, other_copy, self_copy, *this)))
    {
      throw std::runtime_error("arrays not broadcastable!");
    }
  }
  return *this;
}
/**
 * subtract every element from the scalar and return the new output
 * @param scalar to subtract
 * @return new array output
 */
template <typename T, typename C>
typename Tensor<T, C>::SelfType Tensor<T, C>::InlineReverseSubtract(Type const &scalar)
{
  Subtract(scalar, *this, *this);
  return *this;
}

/**
 * multiply other by this array and returns this
 * @param other
 * @return
 */
template <typename T, typename C>
typename Tensor<T, C>::SelfType Tensor<T, C>::InlineMultiply(Tensor const &other)
{
  if (other.shape() == shape_)
  {
    Multiply(*this, other, *this);
  }
  else
  {
    SelfType self_copy  = this->Copy();
    SelfType other_copy = other.Copy();
    if (!(Broadcast([](T x, T y) { return x * y; }, other_copy, self_copy, *this)))
    {
      throw std::runtime_error("arrays not broadcastable!");
    }
  }
  return *this;
}

/**
 * multiplies array by a scalar element wise
 * @param scalar to add
 * @return new array output
 */
template <typename T, typename C>
typename Tensor<T, C>::SelfType Tensor<T, C>::InlineMultiply(Type const &scalar)
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
typename Tensor<T, C>::SelfType Tensor<T, C>::InlineDivide(Tensor const &other)
{
  if (other.shape() == shape_)
  {
    Divide(*this, other, *this);
  }
  else
  {
    SelfType self_copy  = this->Copy();
    SelfType other_copy = other.Copy();
    if (!(Broadcast([](T x, T y) { return x / y; }, self_copy, other_copy, *this)))
    {
      throw std::runtime_error("arrays not broadcastable!");
    }
  }
  return *this;
}

/**
 * Divide array by a scalar elementwise
 * @param scalar to subtract
 * @return new array output
 */
template <typename T, typename C>
typename Tensor<T, C>::SelfType Tensor<T, C>::InlineDivide(Type const &scalar)
{
  Divide(*this, scalar, *this);
  return *this;
}

/**
 * Divide another Tensor by this Tensor from another and support broadcasting
 * @param other
 * @return
 */
template <typename T, typename C>
typename Tensor<T, C>::SelfType Tensor<T, C>::InlineReverseDivide(Tensor const &other)
{
  if (other.shape() == shape_)
  {
    Divide(other, *this, *this);
  }
  else
  {
    SelfType self_copy  = this->Copy();
    SelfType other_copy = other.Copy();
    if (!(Broadcast([](T x, T y) { return x / y; }, other_copy, self_copy, *this)))
    {
      throw std::runtime_error("arrays not broadcastable!");
    }
  }
  return *this;
}

/**
 * Divide scalar by array elementwise
 * @param scalar to subtract
 * @return new array output
 */
template <typename T, typename C>
typename Tensor<T, C>::SelfType Tensor<T, C>::InlineReverseDivide(Type const &scalar)
{
  Divide(scalar, *this, *this);
  return *this;
}

/**
 * + operator
 * @tparam OtherType may be a scalar or array, but must be arithmetic
 * @param other
 * @return
 */
template <typename T, typename C>
template <typename OtherType>
typename Tensor<T, C>::SelfType Tensor<T, C>::operator+(OtherType const &other)
{
  return InlineAdd(other);
}

template <typename T, typename C>
template <typename OtherType>
typename Tensor<T, C>::SelfType Tensor<T, C>::operator+=(OtherType const &other)
{
  return InlineAdd(other);
}

/**
 * + operator
 * @tparam OtherType may be a scalar or array, but must be arithmetic
 * @param other
 * @return
 */
template <typename T, typename C>
template <typename OtherType>
typename Tensor<T, C>::SelfType Tensor<T, C>::operator-(OtherType const &other)
{
  return InlineSubtract(other);
}

template <typename T, typename C>
template <typename OtherType>
typename Tensor<T, C>::SelfType Tensor<T, C>::operator-=(OtherType const &other)
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
typename Tensor<T, C>::SelfType Tensor<T, C>::operator*(OtherType const &other)
{
  return InlineMultiply(other);
}

template <typename T, typename C>
template <typename OtherType>
typename Tensor<T, C>::SelfType Tensor<T, C>::operator*=(OtherType const &other)
{
  return InlineMultiply(other);
}

template <typename T, typename C>
template <typename OtherType>
typename Tensor<T, C>::SelfType Tensor<T, C>::operator/(OtherType const &other)
{
  return InlineDivide(other);
}

template <typename T, typename C>
template <typename OtherType>
typename Tensor<T, C>::SelfType Tensor<T, C>::operator/=(OtherType const &other)
{
  return InlineDivide(other);
}

/**
 * Efficient vectorised and threaded routine for C = A.T(B)
 * @param A
 * @param B
 * @return
 */
template <typename T, typename C>
typename Tensor<T, C>::SelfType &Tensor<T, C>::DotTranspose(SelfType const &A, SelfType const &B,
                                                            Type alpha, Type beta)
{
  ASSERT(this->shape().size() == 2);
  fetch::math::DotTranspose(A, B, *this, alpha, beta);

  return *this;
}

/**
 * Efficient vectorised and threaded routine for C = T(A).B
 * @param A
 * @param B
 * @return
 */
template <typename T, typename C>
typename Tensor<T, C>::SelfType &Tensor<T, C>::TransposeDot(SelfType const &A, SelfType const &B,
                                                            Type alpha, Type beta)
{
  assert(this->shape().size() == 2);
  fetch::math::TransposeDot(A, B, *this, alpha, beta);

  return *this;
}

/**
 *
 * @return
 */
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
void Tensor<T, C>::Exp(SelfType const &x)
{
  Exp(x, *this);
}

/**
 * Calculate the ApproxSoftMax of X and store in this
 */
template <typename T, typename C>
void Tensor<T, C>::ApproxSoftMax(SelfType const &x)
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
  //    if (MajorOrder() == MAJOR_ORDER::COLUMN) {major_order_ = row;}
  //    else {{major_order_ = COLUMN;}}
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

////////////////////////////////////////
/// Tensor methods: Numpy Operations ///
////////////////////////////////////////

/**
 * Copies data from a row major numpy array into the current column major array
 * @param new_array
 */
// TODO(private 869):
template <typename T, typename C>
void Tensor<T, C>::CopyFromNumpy(T *ptr, SizeVector &shape, SizeVector & /*stride*/,
                                 SizeVector & /*index*/)
{
  SizeType total_size = SelfType::SizeFromShape(shape);

  // get pointer to the data
  Resize(total_size);
  assert(this->CanReshape(shape));
  this->Reshape(shape);

  // re-allocate all the data
  TensorIterator<T, C> it(*this);

  // copy all the data initially
  for (SizeType i = 0; i < total_size; ++i)
  {
    *it = ptr[i];
    ++it;
  }

  // numpy arrays are row major - we should be column major by default
  FlipMajorOrder(MAJOR_ORDER::COLUMN);
}

template <typename T, typename C>
void Tensor<T, C>::CopyToNumpy(T *ptr, SizeVector &shape, SizeVector &stride, SizeVector &index)
{

  // copy the data
  TensorIterator<T, C> it(*this);

  for (SizeType j = 0; j < this->size(); ++j)
  {
    // Computing numpy index
    SizeType i   = 0;
    SizeType pos = 0;
    for (i = 0; i < shape.size(); ++i)
    {
      pos += stride[i] * index[i];
    }

    // Updating
    ptr[pos] = *it;
    ++it;

    // Increamenting Numpy
    i = 0;
    ++index[i];
    while (index[i] >= shape[i])
    {
      index[i] = 0;
      ++i;
      if (i >= shape.size())
      {
        break;
      }
      ++index[i];
    }
  }
}

//////////////////////////////
/// Tensor methods: slices ///
//////////////////////////////

/**
 * Returns a Slice that is not permitted to alter the original tensor
 * @tparam T
 * @tparam C
 * @param i
 * @param axis
 * @return
 */
template <typename T, typename C>
typename Tensor<T, C>::ConstSliceType Tensor<T, C>::Slice(SizeType i, SizeType axis) const
{
  std::vector<std::vector<SizeType>> range;

  for (SizeType j = 0; j < shape().size(); ++j)
  {
    if (axis == j)
    {
      range.push_back({i, i + 1, 1});
    }
    else
    {
      range.push_back({0, shape().at(j), 1});
    }
  }

  return ConstSliceType(*this, range, axis);
}

/**
 * Returns a Slice of the tensor
 * @tparam T
 * @tparam C
 * @param i
 * @param axis
 * @return
 */
template <typename T, typename C>
typename Tensor<T, C>::TensorSlice Tensor<T, C>::Slice(SizeType i, SizeType axis)
{
  std::vector<std::vector<SizeType>> range;

  for (SizeType j = 0; j < shape().size(); ++j)
  {
    if (axis == j)
    {
      range.push_back({i, i + 1, 1});
    }
    else
    {
      range.push_back({0, shape().at(j), 1});
    }
  }

  return TensorSlice(*this, range, axis);
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
      ss << At(i) << "\t";
    }
  }
  if (shape_.size() == 2)
  {
    for (SizeType i(0); i < shape_[0]; ++i)
    {
      for (SizeType j(0); j < shape_[1]; ++j)
      {
        ss << At(i, j) << "\t";
      }
      ss << "\n";
    }
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
  return std::numeric_limits<SizeType>::max();
}

/**
 * Stack tensors resulting in a new leading dimension
 * @tparam Tensor
 * @param tensors
 * @return
 */
template <typename T, typename C>
template <typename TensorType>
typename Tensor<T, C>::SelfType Tensor<T, C>::Stack(std::vector<TensorType> const &tensors)
{
  SizeVector retSize;
  retSize.push_back(tensors.size());
  retSize.insert(retSize.end(), tensors.front().shape().begin(), tensors.front().shape().end());
  TensorType ret(retSize);
  for (SizeType i(0); i < tensors.size(); ++i)
  {
    ret.Slice(i).Assign(tensors[i]);
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
void Tensor<T, C>::Sort(memory::TrivialRange const &range)
{
  std::sort(data_.pointer() + range.from(), data_.pointer() + range.to());
}

////////////////

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
    ASSERT(SizeType(index) < shape[N]);
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

template <typename T, typename C>
template <SizeType N, typename TSType>
struct Tensor<T, C>::TensorSetter<N, TSType>
{
  using Type = TSType;

  // Ignore last argument (i.e. value)
  static SizeType IndexOf(SizeVector const &stride, SizeVector const &shape, TSType const &index)
  {
    ASSERT(shape.size() == N);
    ASSERT(stride.size() == N);
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

/// TensorSlice implementations

template <typename T, typename C>
typename Tensor<T, C>::IteratorType Tensor<T, C>::TensorSlice::begin()
{
  auto ret = IteratorType(this->tensor_, this->range_);
  if (this->axis_ != 0)
  {
    ret.MoveAxesToFront(this->axis_);
  }
  return ret;
}

template <typename T, typename C>
typename Tensor<T, C>::IteratorType Tensor<T, C>::TensorSlice::end()
{
  return IteratorType::EndIterator(this->tensor_);
}

template <typename T, typename C>
template <typename G>
void Tensor<T, C>::TensorSlice::Assign(TensorSliceImplementation<G> const &other)
{
  auto it1 = begin();
  auto it2 = other.begin();
  ASSERT(it1.size() == it2.size());
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
  ASSERT(it1.size() == it2.size());
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

/// TensorSliceImplementation implementations

template <typename T, typename C>
template <typename STensor>
typename Tensor<T, C>::SelfType Tensor<T, C>::TensorSliceImplementation<STensor>::Copy() const
{
  SizeVector shape;
  for (SizeType i{0}; i < this->range_.size(); ++i)
  {
    shape.emplace_back(this->range_[i][1] - this->range_[i][0] / this->range_[i][2]);
  }
  SelfType ret{shape};
  ret.Assign(*this);
  return ret;
}

template <typename T, typename C>
template <typename STensor>
typename Tensor<T, C>::ConstIteratorType Tensor<T, C>::TensorSliceImplementation<STensor>::begin()
    const
{
  auto ret = ConstIteratorType(tensor_, range_);
  if (axis_ != 0)
  {
    ret.MoveAxesToFront(axis_);
  }
  return ret;
}

template <typename T, typename C>
template <typename STensor>
typename Tensor<T, C>::ConstIteratorType Tensor<T, C>::TensorSliceImplementation<STensor>::end()
    const
{
  return ConstIteratorType::EndIterator(tensor_);
}

template <typename T, typename C>
template <typename STensor>
STensor &Tensor<T, C>::TensorSliceImplementation<STensor>::Tensor()
{
  return tensor_;
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
}  // namespace fetch
