#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018-2019 Fetch.AI Limited
//
//   Licensed under the Apache License, Version 2.0 (the "License");
//   you may not use this file except in compliance with the License.
//   You may obtain a copy of the License at
//
//       http://www.apache.org/licenses/LICENE-2.0
//
//   Unless required by applicable law or agreed to in writing, software
//   distributed under the License is distributed on an "AS IS" BASIS,
//   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//   See the License for the specific language governing permissions and
//   limitations under the License.
//
//------------------------------------------------------------------------------

// https://github.com/uvue-git/fetch-ledger/tree/cf2fc8441f6ae33d6248559c52473a7f15c5aef2/libs/math/include/math
#include "core/assert.hpp"
#include "math/free_functions/free_functions.hpp"
#include "math/meta/math_type_traits.hpp"
#include "math/tensor_iterator.hpp"
#include "math/shapeless_array.hpp"
#include "math/base_types.hpp"
#include "vectorise/memory/array.hpp"
#include "core/byte_array/const_byte_array.hpp"
#include "core/byte_array/consumers.hpp"

#include <numeric>
#include <utility>
#include <iostream>
#include <memory>
#include <random>

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
  using SizeType             = fetch::math::SizeType;
  using SizeVector           = fetch::math::SizeVector;

  template< typename STensor >
  class TensorSliceImplementation {
  public:
    using Type = typename STensor::Type;

    TensorSliceImplementation(STensor &t, std::vector<std::vector<SizeType>> range, SizeType axis = 0)
    : tensor_{t}
    , range_{std::move(range)}
    , axis_{std::move(axis)}
    {}

    SelfType Copy() const
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

    ConstIteratorType begin() const
    {
      auto ret = ConstIteratorType(tensor_, range_);
      if(axis_ != 0)
      {
        ret.MoveAxesToFront(axis_);
      }
      return ret;
    }

    ConstIteratorType end() const
    {
      return ConstIteratorType::EndIterator(tensor_);
    }

    Tensor Unsqueeze() const
    {
      throw std::runtime_error("TODO: not supported.");
    }

    Tensor const &Tensor() const
    {
      return tensor_;
    }

  protected:
    STensor &tensor_;
    std::vector<std::vector<SizeType>> range_;
    SizeType axis_;

  };

  class TensorSlice : public TensorSliceImplementation< Tensor >
  {
  public:
    using Type = T;
    using TensorSliceImplementation< Tensor >::TensorSliceImplementation;
    using TensorSliceImplementation< Tensor >::begin;
    using TensorSliceImplementation< Tensor >::end;

    IteratorType begin()
    {
      auto ret = IteratorType(this->tensor_, this->range_);
      if(this->axis_ != 0)
      {
        ret.MoveAxesToFront(this->axis_);
      }
      return ret;
    }

    IteratorType end()
    {
      return IteratorType::EndIterator(this->tensor_);
    }

    template< typename X >
    void Assign(TensorSliceImplementation<X> const &other)
    {
      auto it1 = begin();
      auto it2 = other.begin();
      assert(it1.size() == it2.size());
      while(it1.is_valid())
      {
        *it1 = *it2;
        ++it1;
        ++it2;
      }
    }

    void Assign(Tensor const &other)
    {
      auto it1 = begin();
      auto it2 = other.begin();
      assert(it1.size() == it2.size());
      while(it1.is_valid())
      {
        *it1 = *it2;
        ++it1;
        ++it2;
      }
    }

    void Fill(Type t)
    {
      auto it1 = begin();
      while(!it1.is_valid())
      {
        *it1 = t;
        ++it1;
      }
    }

  };

  using ConstSliceType       = TensorSliceImplementation< SelfType const >;
  using SliceType            = TensorSlice;

  enum MAJOR_ORDER
  {
    COLUMN,
    ROW
  };

  Tensor() = default;

  static Tensor FromString(byte_array::ConstByteArray const &c)
  {
    Tensor ret;
    SizeType       n = 1;
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
      ret.ResizeFromShape({n,m});
      ret.SetAllZero();

      SizeType k = 0;
      for (SizeType i = 0; i < n; ++i)
      {
        for (SizeType j = 0; j < m; ++j)
        {
          ret.Set({i, j}, elems[k++]);
        }
      }
    }

    return ret;
  }

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

  void Assign(ConstSliceType const &other)
  {
    auto it1 = begin();
    auto it2 = other.begin();
    ASSERT(it1.size() == it2.size());
    while(it1.is_valid())
    {
      *it1 = *it2;
      ++it1;
      ++it2;
    }
  }

  /**
   * Flattens the array to 1 dimension efficiently
   *
   **/
  void Flatten()
  {
    // TODO: Copy according to new indices
    shape_.clear();
    shape_.push_back(SuperType::size());
    UpdateStrides();
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
  // TODO: Specialise for N = 0 as stride is always 1 for this coordinate
  template< SizeType N, typename FirstIndex, typename ... Indices >
  SizeType UnrollComputeColIndex(FirstIndex &&index, Indices &&... indices) const
  {
    return static_cast<SizeType>(index) * stride_[N] + UnrollComputeColIndex<N + 1>(std::forward<Indices>(indices)...);
  }

  template< SizeType N, typename FirstIndex >
  SizeType UnrollComputeColIndex(FirstIndex &&index ) const
  {
    return static_cast<SizeType>(index) * stride_[N];
  }

  template< typename ... Indices >
  Type &At( Indices ... indices)
  {
    assert(sizeof...(indices) == stride_.size());
    return this->data()[ UnrollComputeColIndex<0>(std::forward<Indices>(indices)...) ] ;
  }  

  template< typename ... Indices >
  Type At( Indices ... indices) const
  {
    assert(sizeof...(indices) == stride_.size());
    SizeType N = UnrollComputeColIndex<0>(std::forward<Indices>(indices)...) ;
    return this->data()[ std::move(N) ] ;
  }    

  /**
   * Operator for accessing data in the array
   *
   * @param[in]     indices specifies the data points to access.
   * @return        the accessed data.
   *
   **/
  template< typename ... Indices >  
  Type operator()(Indices ... indices) const
  {
    return this->At( std::forward< Indices >(indices) ...);
  }

  template< typename ... Indices >  
  Type& operator()(Indices ... indices)
  {
    return this->At( std::forward< Indices >(indices) ...);
  }  

/*
  Type operator()(SizeType const &index) const
  {
    assert(index == this->size_);
    return this->operator[](index);
  }
*/



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

  ConstSliceType Slice(SizeType i, SizeType axis = 0) const
  {
    std::vector< std::vector< SizeType > > range;

    for (SizeType j=0; j < shape().size(); ++j)
    {
      if(axis == j)
      {
        range.push_back({i, i+1, 1});        
      }
      else
      {
        range.push_back({0, shape().at(j), 1});      
      }
    }    

    return ConstSliceType(*this, range, axis);
  }

  TensorSlice Slice(SizeType i, SizeType axis = 0)
  {
    std::vector< std::vector< SizeType > > range;
    range.push_back({i, i+1, 1});

    for (SizeType j=0; j < shape().size(); ++j)
    {
      if(axis == j)
      {
        range.push_back({i, i+1, 1});        
      }
      else
      {
        range.push_back({0, shape().at(j), 1});      
      }
    }    

    return TensorSlice(*this, range, axis);
  }

  Tensor& operator=(ConstSliceType const &slice)
  {
    auto it1 = begin();
    auto it2 = slice.begin();
    assert(it1.size() == it2.size());
    while(it1.is_valid())
    {
      *it1 = *it2;
      ++it1;
      ++it2;
    }
    return *this;
  }

  Tensor& operator=(TensorSlice const & slice)
  {
    auto it1 = begin();
    auto it2 = slice.begin();
    assert(it1.size() == it2.size());
    while(it1.is_valid())
    {
      *it1 = *it2;
      ++it1;
      ++it2;
    }    
    return *this;
  }  


  SelfType Transpose() const
  {
    // TODO: Follow numpy implementation instead which is general and cover N-dimensional tensors
    ASSERT(shape_.size() == 2);
    SizeVector new_axes{1, 0};

    SelfType ret({shape().at(1), shape().at(0)});
    TransposeImplementation(new_axes, ret);
    return ret;
  }

  SelfType Transpose(SizeVector &new_axes) const
  {
    // this implementation is for tensors with more than 2 dimensions
    // TODO: There is no reason why this could not work with two dimensions as well.
    ASSERT(shape_.size() > 2);
    ASSERT(shape_.size() == new_axes.size());

    SelfType ret(shape());
    TransposeImplementation(new_axes, ret);
    return ret;
  }

  SelfType& Squeeze()
  {
    ASSERT(shape_.at(0) == 1);
    shape_.erase(shape_.begin());
    UpdateStrides();

    return *this;
  }

  SelfType& Unsqueeze()
  {
    shape_.insert(shape_.begin(), 1);
    UpdateStrides();

    return *this;
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
    UpdateStrides();    
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
    this->ReshapeForce(shape); // TODO: This construct makes no sense.
  }

  /**
   * Executes a reshape (with no memory checks)
   * @param shape
   */
  // TODO: Work out why this one is here. Seems wrong.
  void ReshapeForce(SizeVector const &shape)
  {
    shape_.clear();
    shape_.reserve(shape.size());
    for (auto const &s : shape)
    {
      shape_.push_back(s);
    }
    UpdateStrides();
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
    auto it1 = this->begin();
    auto it2 = other.begin();
    auto endit = this->end();

    while (it1 != endit)
      {
	*it1 += *it2;
	++it1;
	++it2;
      }
    
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
  MAJOR_ORDER MajorOrder() const
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

  template <typename S>
  friend void Serialize(S &serializer, Tensor const &t) 
  {
    serializer << t.size_;
    serializer << t.shape_;
    // TODO: serialize MAJOR_ORDER
    for(std::size_t i=0; i < t.size(); ++i)
    {
      serializer << t.data()[i];
    }

  }

  template <typename S>
  friend void Deserialize(S &serializer, Tensor &t)
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

  /**
   * useful for printing tensor contents
   * @return
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
  SizeType Find(Type val) const
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
  template <typename TensorType>
  static SelfType Stack(std::vector<TensorType> const &tensors)
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


private:
//  SizeType              size_ = 0;
  SizeVector shape_;
  SizeVector stride_;

  MAJOR_ORDER major_order_ = COLUMN;
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

    // TODO: Reverse order if row major.
  }


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

  void TransposeImplementation(SizeVector &new_axes, SelfType &ret) const
  {
    auto it = this->begin();
    auto eit = this->end();
    auto ret_it = ret.end();
    ret_it.Transpose(new_axes);

    while(it != eit)
    {
      *ret_it = *it;
      ++it;
      ++ret_it;
    }
  }
};







}
}












