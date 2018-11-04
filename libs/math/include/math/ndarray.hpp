#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018 Fetch.AI Limited
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

#include "meta/type_traits.hpp"
#include "math/free_functions/free_functions.hpp"
#include "math/ndarray_iterator.hpp"
#include "math/ndarray_view.hpp"
#include "math/shape_less_array.hpp"
#include "vectorise/memory/array.hpp"

#include <numeric>
#include <utility>
#include <vector>

namespace fetch {
namespace math {

template <typename T, typename C = memory::SharedArray<T>>
class NDArray : public ShapeLessArray<T, C>
{
public:
  using type                 = T;
  using container_type       = C;
  using vector_register_type = typename container_type::vector_register_type;
  using super_type           = ShapeLessArray<T, C>;
  using self_type            = NDArray<T, C>;

  enum MAJOR_ORDER
  {
    COLUMN,
    ROW
  };

  NDArray() = default;

  /**
   * Constructor builds an NDArray with n elements initialized to 0
   * @param n   number of elements in array (no shape specified, assume 1-D)
   */
  NDArray(std::size_t const &n)
    : super_type(n)
  {
    assert(this->size() == n);
    this->LazyReshape({n});
    for (std::size_t idx = 0; idx < this->size(); ++idx)
    {
      this->operator[](idx) = 0;
    }
  }

  /**
   * Constructor builds an empty NDArray pre-initialiing with zeros from a vector of dimension
   * lengths
   * @param shape   vector of lengths for each dimension
   */
  NDArray(std::vector<std::size_t> const &dims)  // : super_type()
  {
    ResizeFromShape(dims);
    this->SetAllZero();
  }

  /**
   * Constructor builds an NDArray pre-initialising from a shapeless array
   * @param arr shapelessarray data set by defualt
   */
  NDArray(super_type const &arr)
    : super_type(arr)
  {
    this->LazyReshape({arr.size()});
  }

  NDArray(self_type const &arr)
    : super_type(arr)
  {
    this->LazyReshape(arr.shape());
  }

  NDArray &operator=(NDArray const &other) = default;
  //  NDArray &operator=(NDArray &&other) = default;

  static std::size_t SizeFromShape(std::vector<std::size_t> const &shape)
  {
    return Product(shape);
    //    return std::accumulate(std::begin(shape), std::end(shape), std::size_t(1),
    //    std::multiplies<>());
  }

  /**
   * Method returning an NDArray of zeroes
   *
   * @param shape : a vector representing the shape of the NDArray
   * @return NDArray with all zeroes
   */
  static self_type Zeroes(std::vector<std::size_t> const &shape)
  {
    std::size_t n = SizeFromShape(shape);
    self_type   output{super_type::Zeroes(n)};
    output.LazyReshape(shape);
    return output;
  }

  /**
   * Method returning an NDArray of ones
   *
   * @param shape : a vector representing the shape of the NDArray
   * @return NDArray with all ones
   */
  static self_type Ones(std::vector<std::size_t> const &shape)
  {
    std::size_t n = std::accumulate(std::begin(shape), std::end(shape), std::size_t(1),
                                    std::multiplies<std::size_t>());
    self_type   output{super_type::Ones(n)};
    output.LazyReshape(shape);
    return output;
  }

  /**
   * Copies input data into current array
   *
   * @param[in]     x is another NDArray of which the data, size, and shape will be copied locally.
   *
   **/
  void Copy(self_type const &x)
  {
    this->super_type::Copy(x);
    this->LazyReshape(x.shape());
  }

  /**
   * equality operators
   *
   * @param other a constant reference to an NDArray to compare against this array
   * @return
   */
  bool operator==(NDArray const &other) const
  {
    if (shape() != other.shape())
    {
      return false;
    }
    return this->super_type::operator==(static_cast<super_type>(other));
  }
  bool operator!=(NDArray const &other) const
  {
    return !(this->operator==(other));
  }

  /**
   * Provides an NDArray that is a copy of the current NDArray
   *
   * @return       copy is a NDArray with the same data, size, and shape of this array.
   *
   **/
  self_type Copy()
  {
    self_type copy = self_type(this->super_type::Copy());

    copy.LazyReshape(this->shape());
    return copy;
  }
  self_type Copy() const
  {
    self_type copy = self_type(this->super_type::Copy());

    copy.LazyReshape(this->shape());
    return copy;
  }

  /**
   * Provides an NDArray that is a copy of a view of the the current NDArray
   *
   * @return       copy is a NDArray with a size and shape equal to or lesser than this array.
   *
   **/
  self_type Copy(NDArrayView arrayView)
  {
    self_type copy;
    return copy(arrayView);
  }
  /**
   * Flattens the array to 1 dimension efficiently
   *
   **/
  void Flatten()
  {
    shape_.clear();
    shape_.push_back(super_type::size());
  }

  /**
   * Operator for accessing data in the array
   *
   * @param[in]     indices specifies the data points to access.
   * @return        the accessed data.
   *
   **/
  type operator()(std::vector<std::size_t> const &indices) const
  {
    assert(indices.size() == shape_.size());
    std::size_t  index = ComputeColIndex(indices);
    return this->operator[](index);
  }
  type operator()(std::size_t const &index) const
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
  fetch::meta::IfIsUnsignedInteger<S, void> Set(std::vector<S> const &indices, type const &val)
  {
    assert(indices.size() == shape_.size());               // dimensionality check not in parent
    this->super_type::Set(ComputeColIndex(indices), val);  // call parent
  }
  template <typename S>
  fetch::meta::IfIsUnsignedInteger<S, void> Set(S const &index, type const &val)
  {
    assert(index < this->size());       // dimensionality check not in parent
    this->super_type::Set(index, val);  // call parent
  }

  /**
   * Gets a value from the array by N-dim index
   * @param indices index to access
   */
  template <typename S>
  fetch::meta::IfIsUnsignedInteger<S, T> Get(std::vector<S> const &indices) const
  {
    assert(indices.size() == shape_.size());
    return this->operator[](ComputeColIndex(indices));
  }
  /**
   * extract data from NDArray based on the NDArrayView
   * @param array_view
   * @return
   */
  self_type GetRange(NDArrayView array_view) const
  {
    std::vector<std::size_t> new_shape;

    // instantiate new array of the right shape
    for (std::size_t cur_dim = 0; cur_dim < array_view.from.size(); ++cur_dim)
    {
      std::size_t cur_from = array_view.from[cur_dim];
      std::size_t cur_to   = array_view.to[cur_dim];
      std::size_t cur_step = array_view.step[cur_dim];
      std::size_t cur_len  = (cur_to - cur_from) / cur_step;

      new_shape.push_back(cur_len);
    }

    // define output
    self_type output = self_type(new_shape);

    // copy all the data
    array_view.recursive_copy(output, *this);

    return output;
  }

  /**
   * extract data from NDArray based on the NDArrayView
   * @param array_view
   * @return
   */
  self_type SetRange(NDArrayView array_view, NDArray new_vals)
  {

    std::vector<std::size_t> new_shape;

    // instantiate new array of the right shape
    for (std::size_t cur_dim = 0; cur_dim < array_view.from.size(); ++cur_dim)
    {
      std::size_t cur_from = array_view.from[cur_dim];
      std::size_t cur_to   = array_view.to[cur_dim];
      std::size_t cur_step = array_view.step[cur_dim];
      std::size_t cur_len  = (cur_to - cur_from) / cur_step;

      new_shape.push_back(cur_len);
    }

    // define output
    self_type output = self_type(new_shape);

    // copy all the data
    array_view.recursive_copy(*this, new_vals);

    return output;
  }

  void ResizeFromShape(std::vector<std::size_t> const &shape)
  {
    this->Resize(self_type::SizeFromShape(shape));
    this->Reshape(shape);
  }

  /**
   * Directly copies shape variable without checking anything
   *
   * @param[in]     shape specifies the new shape.
   *
   **/
  void LazyReshape(std::vector<std::size_t> const &shape)
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
  bool CanReshape(std::vector<std::size_t> const &shape)
  {
    std::size_t total = 1;
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
  void Reshape(std::vector<std::size_t> const &shape)
  {
    assert(CanReshape(shape));
    this->ReshapeForce(shape);
  }

  /**
   * Executes a reshape (with no memory checks)
   * @param shape
   */
  void ReshapeForce(std::vector<std::size_t> const &shape)
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
  std::vector<std::size_t> const &shape() const
  {
    return shape_;
  }
  std::size_t const &shape(std::size_t const &n) const
  {
    return shape_[n];
  }

  /**
   * adds two ndarrays together and supports broadcasting
   * @param other
   * @return
   */
  self_type InlineAdd(NDArray const &other)
  {
    Add(*this, other, *this);
    return *this;
  }
  /**
   * adds a scalar to every element in the array and returns the new output
   * @param scalar to add
   * @return new array output
   */
  self_type InlineAdd(type const &scalar)
  {
    Add(*this, scalar, *this);
    return *this;
  }

  /**
   * Subtract one ndarray from another and support broadcasting
   * @param other
   * @return
   */
  self_type InlineSubtract(NDArray &other)
  {
    Subtract(*this, other, *this);
    return *this;
  }
  /**
   * subtract a scalar from every element in the array and return the new output
   * @param scalar to subtract
   * @return new array output
   */
  self_type InlineSubtract(type const &scalar)
  {
    Subtract(*this, scalar, *this);
    return *this;
  }

  /**
   * multiply other by this array and returns this
   * @param other
   * @return
   */
  self_type InlineMultiply(NDArray &other)
  {
    Multiply(*this, other, *this);
    return *this;
  }
  /**
   * multiplies array by a scalar element wise
   * @param scalar to add
   * @return new array output
   */
  self_type InlineMultiply(type const &scalar)
  {
    Multiply(*this, scalar, *this);
    return *this;
  }

  /**
   * Divide ndarray by another ndarray from another and support broadcasting
   * @param other
   * @return
   */
  self_type InlineDivide(NDArray &other)
  {
    Divide(*this, other, *this);
    return *this;
  }
  /**
   * Divide array by a scalar elementwise
   * @param scalar to subtract
   * @return new array output
   */
  self_type InlineDivide(type const &scalar)
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
  void CopyFromNumpy(T *ptr, std::vector<std::size_t> &shape, std::vector<std::size_t> &stride,
                     std::vector<std::size_t> &index)
  {
    std::size_t total_size = NDArray<T>::SizeFromShape(shape);

    // get pointer to the data
    this->Resize(total_size);
    assert(this->CanReshape(shape));
    this->Reshape(shape);

    // re-allocate all the data
    NDArrayIterator<T, C> it(*this);

    // copy all the data initially
    for (std::size_t i = 0; i < total_size; ++i)
    {
      *it = ptr[i];
      ++it;
    }

    // numpy arrays are row major - we should be column major by default
    FlipMajorOrder(MAJOR_ORDER::COLUMN);
  }

  void CopyToNumpy(T *ptr, std::vector<std::size_t> &shape, std::vector<std::size_t> &stride,
                   std::vector<std::size_t> &index)
  {

    // copy the data
    NDArrayIterator<T, C> it(*this);

    for (std::size_t j = 0; j < this->size(); ++j)
    {
      // Computing numpy index
      std::size_t i   = 0;
      std::size_t pos = 0;
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
  NDArray<T> &DotTranspose(NDArray<T> const &A, NDArray<T> const &B, type alpha = 1.0,
                           type beta = 0.0)
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
  NDArray<T> &TransposeDot(NDArray<T> const &A, NDArray<T> const &B, type alpha = 1.0,
                           type beta = 0.0)
  {
    assert(this->shape().size() == 2);
    fetch::math::TransposeDot(A, B, *this, alpha, beta);

    return *this;
  }

private:
  // TODO(tfr): replace with strides
  std::size_t ComputeRowIndex(std::vector<std::size_t> const &indices) const
  {
    std::size_t index  = 0;
    std::size_t n_dims = indices.size();
    std::size_t base   = 1;

    // loop through all dimensions
    for (std::size_t i = n_dims; i == 0; --i)
    {
      index += indices[i - 1] * base;
      base *= shape_[i - 1];
    }
    return index;
  }

  std::size_t ComputeColIndex(std::vector<std::size_t> const &indices) const
  {
    std::size_t index  = 0;
    std::size_t n_dims = indices.size();
    std::size_t base   = 1;

    // loop through all dimensions
    for (std::size_t i = 0; i < n_dims; ++i)
    {
      index += indices[i] * base;
      base *= shape_[i];
    }
    return index;
  }

  std::size_t              size_ = 0;
  std::vector<std::size_t> shape_;

  MAJOR_ORDER major_order_ = COLUMN;

  /**
   * rearranges data storage in the array. Slow because it copies data instead of pointers
   * @param major_order
   */
  void FlipMajorOrder(MAJOR_ORDER major_order)
  {
    self_type new_array{this->shape()};

    std::vector<std::size_t> stride;
    std::vector<std::size_t> index;

    std::size_t cur_stride = Product(this->shape());

    for (std::size_t i = 0; i < new_array.shape().size(); ++i)
    {
      cur_stride /= this->shape()[i];
      stride.push_back(cur_stride);
      index.push_back(0);
    }

    std::size_t total_size = NDArray<T>::SizeFromShape(new_array.shape());
    NDArrayIterator<T, typename NDArray<T>::container_type> it_this(*this);

    std::size_t cur_dim;
    std::size_t pos;

    if (major_order == MAJOR_ORDER::COLUMN)
    {
      new_array.Copy(*this);
    }

    for (std::size_t j = 0; j < total_size; ++j)
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

}  // namespace math
}  // namespace fetch
