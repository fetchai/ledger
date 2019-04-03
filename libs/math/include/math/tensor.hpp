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


// OLD ND array:
// https://github.com/uvue-git/fetch-ledger/tree/cf2fc8441f6ae33d6248559c52473a7f15c5aef2/libs/math/include/math

#include "math/free_functions/free_functions.hpp"
#include "math/meta/math_type_traits.hpp"
#include "math/tensor_iterator.hpp"
#include "math/tensor_view.hpp"
#include "math/shapeless_array.hpp"
#include "math/base_types.hpp"
#include "vectorise/memory/array.hpp"

#include <numeric>
#include <utility>
#include <vector>



// OLD
#include "core/assert.hpp"
#include "math/free_functions/standard_functions/abs.hpp"
#include "tensor_iterator2.hpp"
#include "shapeless_array.hpp"

#include "core/random/lcg.hpp"

#include <iostream>
#include <memory>
#include <numeric>
#include <random>
#include <vector>

namespace fetch {
namespace math {



template <typename T, typename C = memory::SharedArray<T>>
class Tensor : public ShapelessArray<T, C>
{
public:
  using Type                 = T;
  using ContainerType        = C;
  using vector_register_type = typename ContainerType::vector_register_type; // TODO: Legacy style, replace with new
  using SuperType            = ShapelessArray<T, C>;
  using SelfType             = Tensor<T, C>;
  using IteratorType         = TensorIterator<T, typename SelfType::ContainerType>;
  using ConstIteratorType    = ConstTensorIterator<T, typename SelfType::ContainerType>; 
  using SizeType             = uint64_t;
  using SizeVector           = std::vector< SizeType >;
  // SizeVector

  enum MAJOR_ORDER
  {
    COLUMN,
    ROW
  };

  Tensor() = default;

  /**
   * Constructor builds an Tensor with n elements initialized to 0
   * @param n   number of elements in array (no shape specified, assume 1-D)
   */
  Tensor(SizeType const &n)
    : SuperType(n)
  {
    assert(this->size() == n);
    this->LazyReshape({n});
    Type zero{0};
    for (SizeType idx = 0; idx < this->size(); ++idx)
    {
      this->operator[](idx) = zero;
    }
  }

  /**
   * Constructor builds an empty Tensor pre-initialiing with zeros from a vector of dimension
   * lengths
   * @param shape   vector of lengths for each dimension
   */
  Tensor(SizeVector const &dims)  // : SuperType()
  {
    ResizeFromShape(dims);
    this->SetAllZero();
  }

  /**
   * Constructor builds an Tensor pre-initialising from a shapeless array
   * @param arr shapelessarray data set by defualt
   */
  Tensor(SuperType const &arr)
    : SuperType(arr)
  {
    this->LazyReshape({arr.size()});
  }

  Tensor(SelfType const &arr)
    : SuperType(arr)
  {
    this->LazyReshape(arr.shape());
  }
  Tensor(Tensor &&other) = default;


  Tensor &operator=(Tensor const &other) = default;
  Tensor &operator=(Tensor &&other) = default;

  IteratorType begin()
  {
    return IteratorType(*this);
  }

  IteratorType end()
  {
    return IteratorType::EndIterator(*this);
  }  

  ConstIteratorType begin() const
  {
    return ConstIteratorType(*this);
  }

  ConstIteratorType end() const
  {
    return ConstIteratorType::EndIterator(*this); 
  }  

   ConstIteratorType cbegin() const
  {
    return ConstIteratorType(*this);
  }

  ConstIteratorType cend() const
  {
    return ConstIteratorType::EndIterator(*this); 
  }  

  static SizeType SizeFromShape(SizeVector const &shape)
  {
    return std::accumulate(std::begin(shape), std::end(shape), SizeType(1),
        std::multiplies<>());
  }

  /**
   * Method returning an Tensor of zeroes
   *
   * @param shape : a vector representing the shape of the Tensor
   * @return Tensor with all zeroes
   */
  static SelfType Zeroes(SizeVector const &shape)
  {
    SizeType n = SizeFromShape(shape);
    SelfType   output{SuperType::Zeroes(n)};
    output.LazyReshape(shape);
    return output;
  }

  /**
   * Method returning an Tensor of ones
   *
   * @param shape : a vector representing the shape of the Tensor
   * @return Tensor with all ones
   */
  static SelfType Ones(SizeVector const &shape)
  {
    SizeType n = std::accumulate(std::begin(shape), std::end(shape), SizeType(1),
                                    std::multiplies<SizeType>());
    SelfType   output{SuperType::Ones(n)};
    output.LazyReshape(shape);
    return output;
  }

  /**
   * Copies input data into current array
   *
   * @param[in]     x is another Tensor of which the data, size, and shape will be copied locally.
   *
   **/
  void Copy(SelfType const &x)
  {
    this->SuperType::Copy(x);
    this->LazyReshape(x.shape());
  }

  /**
   * equality operators
   *
   * @param other a constant reference to an Tensor to compare against this array
   * @return
   */
  bool operator==(Tensor const &other) const
  {
    if (shape() != other.shape())
    {
      return false;
    }
    return this->SuperType::operator==(static_cast<SuperType>(other));
  }
  bool operator!=(Tensor const &other) const
  {
    return !(this->operator==(other));
  }

  /**
   * Provides an Tensor that is a copy of the current Tensor
   *
   * @return       copy is a Tensor with the same data, size, and shape of this array.
   *
   **/
  SelfType Copy()
  {
    SelfType copy = SelfType(this->SuperType::Copy());

    copy.LazyReshape(this->shape());
    return copy;
  }

  SelfType Copy() const
  {
    SelfType copy = SelfType(this->SuperType::Copy());

    copy.LazyReshape(this->shape());
    return copy;
  }

  /**
   * Provides an Tensor that is a copy of a view of the the current Tensor
   *
   * @return       copy is a Tensor with a size and shape equal to or lesser than this array.
   *
   **/
  SelfType Copy(TensorView arrayView)
  {
    SelfType copy;
    return copy(arrayView);
  }
  /**
   * Flattens the array to 1 dimension efficiently
   *
   **/
  void Flatten()
  {
    shape_.clear();
    shape_.push_back(SuperType::size());
  }

  /**
   * Operator for accessing data in the array
   *
   * @param[in]     indices specifies the data points to access.
   * @return        the accessed data.
   *
   **/
  Type operator()(SizeVector const &indices) const
  {
    assert(indices.size() == shape_.size());
    SizeType  index = ComputeColIndex(indices);
    return this->operator[](index);
  }
  Type operator()(SizeType const &index) const
  {
    assert(index == size_);
    return this->operator[](index);
  }

  /**
   * Sets a single value in the array using an n-dimensional index
   * @param indices     index position in array
   * @param val         value to write
   */
  // TODO(private issue 123)
  template <typename S>
  fetch::meta::IfIsUnsignedInteger<S, void> Set(std::vector<S> const &indices, Type const &val)
  {
    assert(indices.size() == shape_.size());               // dimensionality check not in parent
    this->SuperType::Set(ComputeColIndex(indices), val);  // call parent
  }


  void Set(SizeVector const &indices, Type const &val)
  {
    assert(indices.size() == shape_.size());               // dimensionality check not in parent
    this->SuperType::Set(ComputeColIndex(indices), val);  // call parent
  }

  /**
   * Gets a value from the array by N-dim index
   * @param indices index to access
   */
  template <typename S>
  fetch::meta::IfIsUnsignedInteger<S, T> &At(std::vector<S> const &indices)
  {
    assert(indices.size() == shape_.size());
    return this->operator[](ComputeColIndex(indices));
  }

  template <typename S>
  fetch::meta::IfIsUnsignedInteger<S, T> const &At(std::vector<S> const &indices) const
  {
    assert(indices.size() == shape_.size());
    return this->operator[](ComputeColIndex(indices));
  }


  
  Type &At(SizeVector const &indices)
  {
    assert(indices.size() == shape_.size());
    return this->operator[](ComputeColIndex(indices));
  }

  Type const &At(SizeVector const &indices) const
  {
    assert(indices.size() == shape_.size());
    return this->operator[](ComputeColIndex(indices));
  }


  /// TODO TODO TODO
  // Introduced for compatibility
  
  Type &At(SizeType const &i)
  {
    return this->data()[i];
  }

  Type const &At(SizeType const &i) const
  {
    return this->data()[i];
  }

  void Set(SizeType const &i, Type val)
  {
    this->data()[i] = val;
  }
  // END TODO END TODO

  /*
  Type &At(std::vector<SizeType> const &indices) 
  {
    assert(indices.size() == shape_.size());
    return this->operator[](ComputeColIndex(indices));
  }

  Type const &At(std::vector<SizeType> const &indices) const
  {
    assert(indices.size() == shape_.size());
    return this->operator[](ComputeColIndex(indices));
  }
  */

  bool AllClose(SelfType const &o, Type const &relative_tolerance = Type(1e-5),
                Type const &absolute_tolerance = Type(1e-8)) const
  {
    // Only enforcing number of elements
    // we allow for different shapes as long as element are in same order
    ASSERT(o.size() == this->size());
    auto it1 = this->begin();
    auto eit1 = this->end();
    auto it2 = o.begin();

    while(it1 != eit1)
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

  SelfType Slice(SizeType /*i*/) const
  {
    SelfType ret;

    throw std::runtime_error("Slice not implemented.");

    return ret;
  }

  SelfType Transpose() const
  {
    throw std::runtime_error("Transpose not implemented.");    
    return SelfType();
  }

  SelfType& Unsqueeze()
  {
    shape_.insert(shape_.begin(), 1);
    return *this;
  }
  
  /**
   * extract data from Tensor based on the TensorView
   * @param array_view
   * @return
   */
  SelfType GetRange(TensorView array_view) const
  {
    SizeVector new_shape;

    // instantiate new array of the right shape
    for (SizeType cur_dim = 0; cur_dim < array_view.from.size(); ++cur_dim)
    {
      SizeType cur_from = array_view.from[cur_dim];
      SizeType cur_to   = array_view.to[cur_dim];
      SizeType cur_step = array_view.step[cur_dim];
      SizeType cur_len  = (cur_to - cur_from) / cur_step;

      new_shape.push_back(cur_len);
    }

    // define output
    SelfType output = SelfType(new_shape);

    // copy all the data
    array_view.recursive_copy(output, *this);

    return output;
  }

  /**
   * extract data from Tensor based on the TensorView
   * @param array_view
   * @return
   */
  SelfType SetRange(TensorView array_view, Tensor new_vals)
  {

    SizeVector new_shape;

    // instantiate new array of the right shape
    for (SizeType cur_dim = 0; cur_dim < array_view.from.size(); ++cur_dim)
    {
      SizeType cur_from = array_view.from[cur_dim];
      SizeType cur_to   = array_view.to[cur_dim];
      SizeType cur_step = array_view.step[cur_dim];
      SizeType cur_len  = (cur_to - cur_from) / cur_step;

      new_shape.push_back(cur_len);
    }

    // define output
    SelfType output = SelfType(new_shape);

    // copy all the data
    array_view.recursive_copy(*this, new_vals);

    return output;
  }

  void ResizeFromShape(SizeVector const &shape)
  {
    this->Resize(SelfType::SizeFromShape(shape));
    this->Reshape(shape);
  }

  /**
   * Directly copies shape variable without checking anything
   *
   * @param[in]     shape specifies the new shape.
   *
   **/
  void LazyReshape(SizeVector const &shape)
  {
    shape_ = shape;
  }

  /**
   * Tests if it is possible to reshape the array to a newly proposed shape
   *
   * @param[in]     shape specified for the new array as a vector ot size_t.
   * @return        success is a bool indicating where the proposed shape is acceptable.
   *
   **/
  bool CanReshape(SizeVector const &shape)
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

  /**
   * Reshapes after checking the total size is the same
   * @param[in]     shape specified for the new array as a vector of size_t.
   *
   **/
  void Reshape(SizeVector const &shape)
  {
    assert(CanReshape(shape));
    this->ReshapeForce(shape);
  }

  /**
   * Executes a reshape (with no memory checks)
   * @param shape
   */
  void ReshapeForce(SizeVector const &shape)
  {
    shape_.clear();
    shape_.reserve(shape.size());
    for (auto const &s : shape)
    {
      shape_.push_back(s);
    }
  }

  /**
   * Returns the array's current shape.
   *
   * @return        shape_ is theshape of the array as a vector of size_t.
   *
   **/
  SizeVector const &shape() const
  {
    return shape_;
  }
  SizeType const &shape(SizeType const &n) const
  {
    return shape_[n];
  }

  /**
   * adds two Tensors together and supports broadcasting
   * @param other
   * @return
   */
  SelfType InlineAdd(Tensor const &other)
  {
    Add(*this, other, *this);
    return *this;
  }
  /**
   * adds a scalar to every element in the array and returns the new output
   * @param scalar to add
   * @return new array output
   */
  SelfType InlineAdd(Type const &scalar)
  {
    Add(*this, scalar, *this);
    return *this;
  }

  /**
   * Subtract one Tensor from another and support broadcasting
   * @param other
   * @return
   */
  SelfType InlineSubtract(Tensor &other)
  {
    Subtract(*this, other, *this);
    return *this;
  }
  /**
   * subtract a scalar from every element in the array and return the new output
   * @param scalar to subtract
   * @return new array output
   */
  SelfType InlineSubtract(Type const &scalar)
  {
    Subtract(*this, scalar, *this);
    return *this;
  }

  /**
   * multiply other by this array and returns this
   * @param other
   * @return
   */
  SelfType InlineMultiply(Tensor &other)
  {
    Multiply(*this, other, *this);
    return *this;
  }
  /**
   * multiplies array by a scalar element wise
   * @param scalar to add
   * @return new array output
   */
  SelfType InlineMultiply(Type const &scalar)
  {
    Multiply(*this, scalar, *this);
    return *this;
  }

  /**
   * Divide Tensor by another Tensor from another and support broadcasting
   * @param other
   * @return
   */
  SelfType InlineDivide(Tensor &other)
  {
    Divide(*this, other, *this);
    return *this;
  }
  /**
   * Divide array by a scalar elementwise
   * @param scalar to subtract
   * @return new array output
   */
  SelfType InlineDivide(Type const &scalar)
  {
    Divide(*this, scalar, *this);
    return *this;
  }

  void MajorOrderFlip()
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
   * Copies data from a row major numpy array into the current column major array
   * @param new_array
   */
  // TODO: Get rid of this 
  void CopyFromNumpy(T *ptr, SizeVector &shape, SizeVector & /*stride*/,
                     SizeVector & /*index*/)
  {
    SizeType total_size = SelfType::SizeFromShape(shape);

    // get pointer to the data
    this->Resize(total_size);
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

  void CopyToNumpy(T *ptr, SizeVector &shape, SizeVector &stride,
                   SizeVector &index)
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

  /**
   * returns the current major order of the array
   * @return
   */
  MAJOR_ORDER MajorOrder()
  {
    return major_order_;
  }

  /**
   * Efficient vectorised and threaded routine for C = A.T(B)
   * @param A
   * @param B
   * @return
   */
  SelfType &DotTranspose(SelfType const &A, SelfType const &B, Type alpha = 1.0,
                           Type beta = 0.0)
  {
    assert(this->shape().size() == 2);
    fetch::math::DotTranspose(A, B, *this, alpha, beta);

    return *this;
  }

  /**
   * Efficient vectorised and threaded routine for C = T(A).B
   * @param A
   * @param B
   * @return
   */
  SelfType &TransposeDot(SelfType const &A, SelfType const &B, Type alpha = 1.0,
                           Type beta = 0.0)
  {
    assert(this->shape().size() == 2);
    fetch::math::TransposeDot(A, B, *this, alpha, beta);

    return *this;
  }

  Type Sum() const
  {
    Type ret{0};
    for(auto const &v: *this)
    {
      ret += v;
    }
    return ret;
  }

  template <typename S, typename U, typename V>
  friend void Serialize(S &serializer, Tensor<U, V> const &t) 
  {
    serializer << t.size_;
    serializer << t.shape_;
    // TODO: serialize MAJOR_ORDER
    for(std::size_t i=0; i < t.size(); ++i)
    {
      serializer << t.data()[i];
    }

  }

  template <typename S, typename U, typename V>
  friend void Deserialize(S &serializer, Tensor<U, V> &t)
  {
    SizeType size;
    SizeVector shape;
    serializer >> size;
    serializer >> shape;

    t.Resize(size);
    t.Reshape(shape);

    for(std::size_t i=0; i < t.size(); ++i)
    {
      serializer >> t.data()[i];
    }

  } 

private:
  SizeType              size_ = 0;
  SizeVector shape_;
  MAJOR_ORDER major_order_ = COLUMN;

  // TODO(tfr): replace with strides
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

    SizeType total_size = SelfType::SizeFromShape(new_array.shape());
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
};





















template <typename T>
class Tensor2
{
public:
  using Type                             = T;
  using SizeType                         = std::uint64_t;
  using SelfType                         = Tensor2<T>;
  using SharedArray                      = memory::SharedArray<T>;
  using StorageType                      = ShapelessArray<T, SharedArray >;
  using SizeVector                       = std::vector< SizeType >;
  static const SizeType DefaultAlignment = 8;  // Arbitrary picked

public:
  Tensor2(SizeVector            shape   = SizeVector (),
         SizeVector            strides = SizeVector (),
         SizeVector            padding = SizeVector (),
         StorageType           storage = StorageType{}, 
         SizeType               offset = 0)
    : shape_(std::move(shape))
    , padding_(std::move(padding))
    , input_strides_(std::move(strides))
    , storage_(std::move(storage))
    , offset_(offset)
  {
    ASSERT(padding.empty() || padding.size() == shape.size());
    ASSERT(strides.empty() || strides.size() == shape.size());
    Init(input_strides_, padding_);
  }

  Tensor2(SizeType size)
    : shape_({size})
  {
    Init(strides_, padding_);
  }

  Tensor2(Tensor2 const &t)     = default;
  Tensor2(Tensor2 &&t) = default;
  Tensor2 &operator=(Tensor2 const &other) = default;
  Tensor2 &operator=(Tensor2 &&) = default;

  template <typename S, typename U>
  friend void Serialize(S &serializer, Tensor2<U> const &t) 
  {
    serializer << t.shape_;
    serializer << t.strides_;
    serializer << t.padding_;
    serializer << t.offset_;

    serializer << t.data().size();
    for(std::size_t i=0; i < t.data().size(); ++i)
    {
      serializer << t.data()[i];
    }

  }

  template <typename S, typename U>
  friend void Deserialize(S &serializer, Tensor2<U> &t)
  {
    using StorageType = typename math::Tensor2<U>::StorageType;  

    serializer >> t.shape_;
    serializer >> t.strides_;
    serializer >> t.padding_;
    serializer >> t.offset_;
    serializer >> t.size_;

    StorageType storage;
    storage.Resize(t.size_);

    for(std::size_t i=0; i < t.data().size(); ++i)
    {
      serializer >> storage[i];
    }
    t.storage_ = storage;

  //  t = math::Tensor2<U>(shape, strides, padding, storage, offset);
  }  
  /**
   * Returns a deep copy of this tensor2
   * @return
   */

  SelfType Clone() const
  {
    SelfType copy;

    copy.shape_   = this->shape_;
    copy.padding_ = this->padding_;
    copy.strides_ = this->strides_;
    copy.offset_  = this->offset_;
    copy.size_    = this->size_;

    copy.storage_ = storage_.Copy();

    return copy;
  }

  /**
   * Copy data from another tensor2 into this one
   * assumes relevant stride/offset etc. are still applicable
   * @param other
   * @return
   */
  void Copy(SelfType const &other)
  {
    assert(other.size() == this->size());

    storage_ = other.storage_.Copy();
  }

  // TODO(private, 520) fix capitalisation (kepping it consistent with Tensor for now)
  SizeVector  const &shape() const
  {
    return shape_;
  }

  SizeType Capacity() const
  {
    return storage_.size();
  }

  // TODO(private, 520): fix capitalisation (kepping it consistent with Tensor for now)
  SizeType size() const
  {
    return size_;
  }

  /**
   * Return the coordinates of the nth element in N dimensions
   * @param element     ordinal position of the element we want
   * @return            coordinate of said element in the tensor2
   */
  SizeVector  IndicesOfElement(SizeType element) const
  {
    ASSERT(element < size());
    SizeVector  results(shape_.size());
    results.back() = element;
    for (SizeType i(shape_.size() - 1); i > 0; --i)
    {
      results[i - 1] = results[i] / shape_[i];
      results[i] %= shape_[i];
    }
    return results;
  }

  /**
   * Return the offset of element at specified coordinates in the low level memory array
   * @param indices     coordinate of requested element in the tensor2
   * @return            offset in low level memory array
   */
  SizeType OffsetOfElement(SizeVector  const &indices) const
  {
    SizeType index(offset_);
    for (SizeType i(0); i < indices.size(); ++i)
    {
      ASSERT(indices[i] < shape_[i]);
      index += indices[i] * DimensionSize(i);
    }
    return index;
  }

  void Fill(T const &value)
  {
    for (SizeType i(0); i < size(); ++i)
    {
      At(i) = value;
    }
  }



  /////////////////
  /// Iterators ///
  /////////////////

  TensorIterator2<T, SizeType> begin() const  // Need to stay lowercase for range basedloops
  {
    return TensorIterator2<T, SizeType>(shape_, strides_, padding_,
                                       SizeVector (shape_.size()), storage_, offset_);
  }

  TensorIterator2<T, SizeType> end() const  // Need to stay lowercase for range basedloops
  {
    SizeVector  endCoordinate(shape_.size());
    endCoordinate[0] = shape_[0];
    return TensorIterator2<T, SizeType>(shape_, strides_, padding_, endCoordinate, storage_,
                                       offset_);
  }

  /////////////////
  /// ACCESSORS ///
  /////////////////

  T &At(SizeType i)
  {
    return storage_[OffsetOfElement(IndicesOfElement(i))];
  }

  T const &At(SizeType i) const
  {
    return storage_[OffsetOfElement(IndicesOfElement(i))];
  }

  T const &operator()(SizeVector  const &indices) const
  {
    return At(indices);
  }

  T const &At(SizeVector  const &indices) const
  {
    return storage_[OffsetOfElement(indices)];
  }

  T &At(SizeVector  const &indices)
  {
    return storage_[OffsetOfElement(indices)];
  }

  ///////////////
  /// SETTERS ///
  ///////////////

  void Set(SizeVector  const &indices, T value)
  {
    storage_[OffsetOfElement(indices)] = value;
  }

  void Set(SizeType i, T value)
  {
    storage_[OffsetOfElement(IndicesOfElement(i))] = value;
  }

  T &operator[](SizeType i)
  {
    return At(i);
  }

  T const &operator[](SizeType i) const
  {
    return At(i);
  }

  /*
   * return a slice of the tensor2 along the first dimension
   */
  Tensor2<T> Slice(SizeType i) const
  {
    assert(shape_.size() > 1 && i < shape_[0]);
    Tensor2<T> ret(SizeVector (std::next(shape_.begin()), shape_.end()),     /* shape */
                  SizeVector (std::next(strides_.begin()), strides_.end()), /* stride */
                  SizeVector (std::next(padding_.begin()), padding_.end()), /* padding */
                  storage_, offset_ + i * DimensionSize(0));
    ret.strides_ = SizeVector (std::next(strides_.begin()), strides_.end());
    ret.padding_ = SizeVector (std::next(padding_.begin()), padding_.end());
    return ret;
  }

  /*
   * Add a dummy leading dimension
   * Ex: [4, 5, 6].Unsqueeze() -> [1, 4, 5, 6]
   */
  
  Tensor2<T> &Unsqueeze()
  {
    shape_.insert(shape_.begin(), 1);
    strides_.insert(strides_.begin(), strides_.front() * shape_[1]);
    padding_.insert(padding_.begin(), 0);
    return *this;
  }
  
  /*
   * Inverse of unsqueze : Collapse a empty leading dimension
   */
  /*
  Tensor2<T>& Squeeze()
  {
    if (shape_.front() == 1)
    {
      shape_.erase(shape_.begin());
      strides_.erase(strides_.begin());
      padding_.erase(padding_.begin());
    }
    else
    {
      throw std::runtime_error("Can't squeeze tensor2 with leading dimension of size " +
                               std::to_string(shape_[0]));
    }
    return *this;
  }
*/
  bool AllClose(Tensor2<T> const &o, T const &relative_tolerance = T(1e-5),
                T const &absolute_tolerance = T(1e-8)) const
  {
    // Only enforcing number of elements
    // we allow for different shapes as long as element are in same order
    ASSERT(o.size() == size());
    for (SizeType i(0); i < size(); ++i)
    {
      T e1 = At(IndicesOfElement(i));
      T e2 = o.At(o.IndicesOfElement(i));

      T abs_e1 = e1;
      fetch::math::Abs(abs_e1, abs_e1);
      T abs_e2 = e2;
      fetch::math::Abs(abs_e2, abs_e2);
      T abs_diff = e1 - e2;
      fetch::math::Abs(abs_diff, abs_diff);
      T tolerance = std::max(absolute_tolerance, std::max(abs_e1, abs_e2) * relative_tolerance);
      if (abs_diff > tolerance)
      {
        std::cout << "AllClose[" << i << "] - " << e1 << " != " << e2 << std::endl;
        return false;
      }
    }
    return true;
  }

  Tensor2<T> &InlineAdd(T const &o)
  {
    for (T &e : *this)
    {
      e += o;
    }
    return *this;
  }

  Tensor2<T> &InlineAdd(Tensor2<T> const &o)
  {
    assert(size() == o.size());
    auto it1 = this->begin();
    auto end = this->end();
    auto it2 = o.begin();

    while (it1 != end)
    {
      *it1 += *it2;
      ++it1;
      ++it2;
    }
    return *this;
  }

  Tensor2<T> &InlineSubtract(T const &o)
  {
    for (T &e : *this)
    {
      e -= o;
    }
    return *this;
  }

  Tensor2<T> &InlineSubtract(Tensor2<T> const &o)
  {
    assert(size() == o.size());
    auto it1 = this->begin();
    auto end = this->end();
    auto it2 = o.begin();

    while (it1 != end)
    {
      *it1 -= *it2;
      ++it1;
      ++it2;
    }
    return *this;
  }

  Tensor2<T> &InlineReverseSubtract(Tensor2<T> const &o)
  {
    assert(size() == o.size());
    for (SizeType i(0); i < size(); ++i)
    {
      At(i) = o.At(i) - At(i);
    }
    return *this;
  }

  Tensor2<T> &InlineMultiply(T const &o)
  {
    for (T &e : *this)
    {
      e *= o;
    }
    return *this;
  }

  Tensor2<T> &InlineMultiply(Tensor2<T> const &o)
  {
    assert(size() == o.size());
    auto it1 = this->begin();
    auto end = this->end();
    auto it2 = o.begin();

    while (it1 != end)
    {
      *it1 *= *it2;
      ++it1;
      ++it2;
    }
    return *this;
  }

  Tensor2<T> &InlineDivide(T const &o)
  {
    for (T &e : *this)
    {
      e /= o;
    }
    return *this;
  }

  Tensor2<T> &InlineDivide(Tensor2<T> const &o)
  {
    assert(size() == o.size());
    auto it1 = this->begin();
    auto end = this->end();
    auto it2 = o.begin();

    while (it1 != end)
    {
      *it1 /= *it2;
      ++it1;
      ++it2;
    }
    return *this;
  }

  T Sum() const
  {
    return std::accumulate(begin(), end(), T(0));
  }
  

  Tensor2<T> Transpose() const
  {
    assert(shape_.size() == 2);
    Tensor2<T> ret(SizeVector ({shape_[1], shape_[0]}), // shape 
                  SizeVector (),                       // stride 
                  SizeVector (),                       // padding 
                  storage_, offset_);
    ret.strides_ = SizeVector (strides_.rbegin(), strides_.rend());
    ret.padding_ = SizeVector (padding_.rbegin(), padding_.rend());
    return ret;
  }

  /**
   * randomly reassigns the data within the tensor2 - expensive method since it requires data copy
   */
/*  
  void Shuffle() // TODO: Really poor implementation
  {
    std::default_random_engine rng{};
    SizeVector       idxs(size());
    std::iota(std::begin(idxs), std::end(idxs), 0);
    std::shuffle(idxs.begin(), idxs.end(), rng);

    // instantiate new tensor2 with copy of data
    Tensor2<Type> tmp = this->Clone();

    // copy data back according to shuffle
    for (std::size_t j = 0; j < tmp.size(); ++j)
    {
      this->Set(j, tmp.At(idxs[j]));
    }
  }
  */

  std::string ToString() const
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
          ss << At({i, j}) << "\t";
        }
        ss << "\n";
      }
    }
    return ss.str();
  }

  //////////////////////
  /// equality check ///
  //////////////////////

  /**
   * equality operator for tensor2s. checks size, shape, and data.
   * Fast when tensor2s not equal, slow otherwise
   * @param other
   * @return
   */
  bool operator==(Tensor2 const &other) const
  {
    bool ret = false;
    if ((this->size() == other.size()) && (this->shape_ == other.shape()))
    {
      ret = this->AllClose(other);
    }
    return ret;
  }

  bool operator!=(Tensor2 const &other) const
  {
    return !(*this == other);
  }

  SharedArray data() { return storage_.data(); }
  SharedArray const& data() const { return storage_.data(); }  
private:
  SizeType DimensionSize(SizeType dim) const
  {
    if (!shape_.empty() && dim < shape_.size())
    {
      return strides_[dim];
    }
    return 0;
  }


  /**
   * Initialises default values for stride padding etc.
   * @param strides
   * @param padding
   */
  void Init(SizeVector  const &strides = SizeVector (),
            SizeVector  const &padding = SizeVector ())
  {
    if (!shape_.empty())
    {
      if (strides.empty())
      {
        strides_ = SizeVector (shape_.size(), 1);
      }
      else
      {
        strides_ = strides;
      }

      if (padding.empty())
      {
        padding_        = SizeVector (shape_.size(), 0);
        padding_.back() = DefaultAlignment - ((strides_.back() * shape_.back()) % DefaultAlignment);
      }

      SizeType dim = 1;
      for (SizeType i(shape_.size()); i-- > 0;)
      {
        dim *= strides_[i];
        strides_[i] = dim;
        dim *= shape_[i];
        dim += padding_[i];
      }

      if (storage_.size() == 0)
      {
        offset_ = 0;
        if (!shape_.empty())
        {
          storage_.Resize(std::max(SizeType(1), DimensionSize(0) * shape_[0] + padding_[0]));
        }
      }
    }

    // TODO: You gotta be fucking kidding me!
    if (shape_.empty())
    {
      size_ = 0;
    }
    else
    {
      SizeType n(1);
      for (SizeType d : shape_)
      {
        n *= d;
      }
      size_ = n;
    }
  }


  SizeVector                      shape_;
  SizeVector                      padding_;
  SizeVector                      strides_;
  SizeVector                      input_strides_;
  StorageType                     storage_{};
  SizeType                        offset_{0};
  SizeType                        size_{0};
};
}  // namespace math
}  // namespace fetch
