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
#include "math/ndarray_view.hpp"
#include "math/shape_less_array.hpp"
#include "math/statistics/max.hpp"
#include "math/statistics/min.hpp"
#include "vectorise/memory/array.hpp"

#include "math/ndarray_broadcast.hpp"

#include <numeric>
#include <utility>
#include <vector>

namespace fetch {
namespace math {

template <typename T, typename C = memory::SharedArray<T>>
class NDArray : public ShapeLessArray<T, C>
{
public:
  using data_type            = T;
  using container_type       = C;
  using vector_register_type = typename container_type::vector_register_type;
  using super_type           = ShapeLessArray<T, C>;
  using self_type            = NDArray<T, C>;

  NDArray() = default;

  /**
   * Constructor builds an NDArray with n elements initialized to 0
   * @param n   number of elements in array (no shape specified, assume 1-D)
   */
  NDArray(std::size_t const &n) : super_type(n)
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
  NDArray(super_type const &arr) : super_type(arr) { this->LazyReshape({arr.size()}); }

  NDArray(self_type const &arr) : super_type(arr) { this->LazyReshape(arr.shape()); }

  NDArray &operator=(NDArray const &other) = default;
  //  NDArray &operator=(NDArray &&other) = default;

  static std::size_t SizeFromShape(std::vector<std::size_t> const &shape)
  {
    return std::accumulate(std::begin(shape), std::end(shape), std::size_t(1), std::multiplies<>());
  }

  static self_type Zeroes(std::vector<size_t> const &n)
  {
    self_type ret;
    ret.Reshape(n);
    ret.SetAllZero();
    return ret;
  }



  void ResizeFromShape(std::vector<std::size_t> const &shape)
  {
    this->Resize(self_type::SizeFromShape(shape));
    this->Reshape(shape);
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
    if (shape() != other.shape()) return false;
    return this->super_type::operator==(static_cast<super_type>(other));
  }
  bool operator!=(NDArray const &other) const { return !(this->operator==(other)); }

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
   * Directly copies shape variable without checking anything
   *
   * @param[in]     shape specifies the new shape.
   *
   **/
  void LazyReshape(std::vector<std::size_t> const &shape) { shape_ = shape; }

  /**
   * Operator for accessing data in the array
   *
   * @param[in]     indices specifies the data points to access.
   * @return        the accessed data.
   *
   **/
  data_type operator()(std::vector<std::size_t> const &indices) const
  {
    assert(indices.size() == shape_.size());
    std::size_t  index = ComputeColIndex(indices);
    return this->operator[](index);
  }
  data_type operator()(std::size_t const &index) const
  {
    assert(index == size_);
    return this->operator[](index);
  }

  /**
   * Sets a single value in the array using an n-dimensional index
   * @param indices     index position in array
   * @param val         value to write
   */
  void Set(std::vector<std::size_t> const &indices, data_type const &val)
  {
    assert(indices.size() == shape_.size());               // dimensionality check not in parent
    this->super_type::Set(ComputeColIndex(indices), val);  // call parent
  }
  /**
   * Gets a value from the array by N-dim index
   * @param indices index to access
   */
  data_type Get(std::vector<std::size_t> const &indices) const
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
   * Reshapes the array to the shape specified.
   *
   * @param[in]     shape specified for the new array as a vector of size_t.
   *
   **/
  void Reshape(std::vector<std::size_t> const &shape)
  {
    assert(CanReshape(shape));

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
  std::vector<std::size_t> const &shape() const { return shape_; }
  std::size_t const &             shape(std::size_t const &n) const { return shape_[n]; }

  /**
   * Returns the single maximum value in the array
   * @return
   */
  data_type Max() const { return fetch::math::statistics::Max(*this); }
  self_type Max(std::size_t const axis)
  {
    std::vector<std::size_t> return_shape{shape()};
    return_shape.erase(return_shape.begin() + int(axis), return_shape.begin() + int(axis) + 1);
    self_type                                  ret{return_shape};
    NDArrayIterator<data_type, container_type> return_iterator{ret};

    // iterate through the return array (i.e. the array of Max vals)
    //    data_type cur_val;
    std::vector<std::size_t> cur_index;
    while (return_iterator)
    {
      std::vector<std::vector<std::size_t>> cur_step;

      cur_index = return_iterator.GetNDimIndex();

      // calculate step from cur_index and axis
      std::size_t index_counter = 0;
      for (std::size_t i = 0; i < shape().size(); ++i)
      {
        if (i == axis)
        {
          cur_step.push_back({0, shape()[i]});
        }
        else
        {
          cur_step.push_back({cur_index[index_counter], cur_index[index_counter] + 1});
          ++index_counter;
        }
      }

      // get an iterator to iterate over the 1-d slice of the array to calculate max over
      NDArrayIterator<data_type, container_type> array_iterator(*this, cur_step);

      // loops through the 1d array calculating the max val
      data_type cur_max = -std::numeric_limits<data_type>::max();
      data_type cur_val;
      while (array_iterator)
      {
        cur_val = *array_iterator;
        cur_max = fetch::math::statistics::Max(cur_max, cur_val);
        ++array_iterator;
      }

      *return_iterator = cur_max;
      ++return_iterator;
    }

    return ret;
  }

  /**
   * Returns the single minimum value in the array
   * @return
   */
  data_type Min() const { return fetch::math::statistics::Min(*this); }
  self_type Min(std::size_t const axis)
  {
    std::vector<std::size_t> return_shape{shape()};
    return_shape.erase(return_shape.begin() + int(axis), return_shape.begin() + int(axis) + 1);

    assert(axis < return_shape.size());

    self_type                                  ret{return_shape};
    NDArrayIterator<data_type, container_type> return_iterator{ret};

    // iterate through the return array (i.e. the array of Max vals)
    //    data_type cur_val;
    std::vector<std::size_t> cur_index;
    while (return_iterator)
    {
      std::vector<std::vector<std::size_t>> cur_step;

      cur_index = return_iterator.GetNDimIndex();

      // calculate step from cur_index and axis
      std::size_t index_counter = 0;
      for (std::size_t i = 0; i < shape().size(); ++i)
      {
        if (i == axis)
        {
          cur_step.push_back({0, shape()[i]});
        }
        else
        {
          cur_step.push_back({cur_index[index_counter], cur_index[index_counter] + 1});
          ++index_counter;
        }
      }

      // get an iterator to iterate over the 1-d slice of the array to calculate max over
      NDArrayIterator<data_type, container_type> array_iterator(*this, cur_step);

      // loops through the 1d array calculating the max val
      data_type cur_max = std::numeric_limits<data_type>::max();
      data_type cur_val;
      while (array_iterator)
      {
        cur_val = *array_iterator;
        cur_max = fetch::math::statistics::Min(cur_max, cur_val);
        ++array_iterator;
      }

      *return_iterator = cur_max;
      ++return_iterator;
    }

    return ret;
  }
  /**
   * adds two ndarrays together and supports broadcasting
   * @param other
   * @return
   */
  static NDArray Add(self_type &obj1, self_type &other)
  {
    self_type ret;
    Broadcast([](data_type x, data_type y) { return x + y; }, obj1, other, ret);
    return ret;
  }
  /**
   * adds a scalar to every element in the array and returns the new output
   * @param scalar to add
   * @return new array output
   */
  self_type Add(self_type const &obj1, data_type const &scalar)
  {
    return self_type(super_type::Add(obj1, scalar));
  }
  /**
   * adds two ndarrays together and supports broadcasting
   * @param other
   * @return
   */
  self_type InlineAdd(NDArray &other)
  {
    Broadcast([](data_type x, data_type y) { return x + y; }, *this, other, *this);
    return *this;
  }
  /**
   * adds a scalar to every element in the array and returns the new output
   * @param scalar to add
   * @return new array output
   */
  self_type InlineAdd(data_type const &scalar) { return self_type(super_type::InlineAdd(scalar)); }

  /**
   * Subtract one ndarray from another and support broadcasting
   * @param other
   * @return
   */
  static NDArray Subtract(self_type &obj1, self_type &other)
  {
    self_type ret;
    Broadcast([](data_type x, data_type y) { return x - y; }, obj1, other, ret);
    return ret;
  }
  /**
   * subtract a scalar from every element in the array and return the new output
   * @param other
   * @return
   */
  self_type Subtract(self_type &obj1, data_type const &scalar)
  {
    return self_type(super_type::Subtract(obj1, scalar));
  }
  /**
   * Subtract one ndarray from another and support broadcasting
   * @param other
   * @return
   */
  self_type InlineSubtract(NDArray &other)
  {
    Broadcast([](data_type x, data_type y) { return x - y; }, *this, other, *this);
    return *this;
  }
  /**
   * subtract a scalar from every element in the array and return the new output
   * @param scalar to subtract
   * @return new array output
   */
  self_type InlineSubtract(data_type const &scalar)
  {
    return self_type(super_type::InlineSubtract(scalar));
  }

  /**
   * multiplies two ndarrays together and supports broadcasting
   * @param other
   * @return
   */
  static NDArray Multiply(self_type &obj1, self_type &other)
  {
    self_type ret;
    Broadcast([](data_type x, data_type y) { return x * y; }, obj1, other, ret);
    return ret;
  }
  /**
   * multiplies array by a scalar element wise
   * @param other
   * @return
   */
  self_type Multiply(self_type &obj1, data_type const &scalar)
  {
    return self_type(super_type::Multiply(obj1, scalar));
  }
  /**
   * multiplies two ndarrays together and supports broadcasting
   * @param other
   * @return
   */
  self_type InlineMultiply(NDArray &other)
  {
    Broadcast([](data_type x, data_type y) { return x * y; }, *this, other, *this);
    return *this;
  }
  /**
   * multiplies array by a scalar element wise
   * @param scalar to add
   * @return new array output
   */
  self_type InlineMultiply(data_type const &scalar)
  {
    return self_type(super_type::InlineMultiply(scalar));
  }

  /**
   * Divide ndarray by another ndarray from another and support broadcasting
   * @param other
   * @return
   */
  static NDArray Divide(self_type &obj1, self_type &other)
  {
    self_type ret;
    Broadcast([](data_type x, data_type y) { return x / y; }, obj1, other, ret);
    return ret;
  }
  /**
   * Divide array by a scalar elementwise
   * @param other
   * @return
   */
  self_type Divide(self_type &obj1, data_type const &scalar)
  {
    return self_type(super_type::Divide(obj1, scalar));
  }
  /**
   * Divide ndarray by another ndarray from another and support broadcasting
   * @param other
   * @return
   */
  self_type InlineDivide(NDArray &other)
  {
    Broadcast([](data_type x, data_type y) { return x / y; }, *this, other, *this);
    return *this;
  }
  /**
   * Divide array by a scalar elementwise
   * @param scalar to subtract
   * @return new array output
   */
  self_type InlineDivide(data_type const &scalar)
  {
    return self_type(super_type::InlineDivide(scalar));
  }

private:
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
};
}  // namespace math
}  // namespace fetch
