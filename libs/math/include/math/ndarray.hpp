#pragma once
#include "math/ndarray_view.hpp"
#include "math/shape_less_array.hpp"
#include "math/statistics/max.hpp"
#include "math/statistics/min.hpp"
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

  /**
   * Constructor builds an NDArray with n elements initialized to 0
   * @param n   number of elements in array (no shape specified, assume 1-D)
   */
  NDArray(std::size_t const &n = 0) : super_type(n)
  {
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
  NDArray(std::vector<std::size_t> const &dims = {0})
    : super_type(std::accumulate(std::begin(dims), std::end(dims), std::size_t(1),
                                 std::multiplies<std::size_t>()))
  {
    this->LazyReshape(dims);
    for (std::size_t idx = 0; idx < this->size(); ++idx)
    {
      this->operator[](idx) = 0;
    }
  }

  /**
   * Constructor builds an NDArray pre-initialising from a shapeless array
   * @param arr shapelessarray data set by defualt
   */
  NDArray(super_type const &arr) : super_type(arr) {}

  NDArray &operator=(NDArray const &other) = default;
  //  NDArray &operator=(NDArray &&other) = default;

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
  type operator()(std::vector<std::size_t> indices) const
  {
    assert(indices.size() == shape_.size());
    std::size_t  index = ComputeColIndex(indices);
    return this->operator[](index);
  }
  type operator()(std::size_t index) const
  {
    assert(index == size_);
    return this->operator[](index);
  }

  void Set(std::vector<std::size_t> indices, type val)
  {
    assert(indices.size() == shape_.size());
    this->AssignVal(ComputeColIndex(indices), val);
  }
  type Get(std::vector<std::size_t> indices) const
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

  type      Max() const { return this->super_type::Max(); }
  self_type Max(std::size_t fixed_axis) const
  {
    // set up the new output array
    std::vector<std::size_t> new_shape{shape()};
    new_shape.erase(new_shape.begin() + int(fixed_axis));
    NDArray array_out{new_shape};

    for (std::size_t i = 0; i < shape()[fixed_axis]; ++i)
    {

      // set up the view to extract
      NDArrayView arr_view;

      for (std::size_t j = 0; j < shape().size(); ++j)
      {
        if (j == fixed_axis)
        {
          arr_view.from.push_back(i);
          arr_view.to.push_back(i + 1);
          arr_view.step.push_back(1);
        }
        else
        {
          arr_view.from.push_back(0);
          arr_view.to.push_back(shape()[j]);
          arr_view.step.push_back(1);
        }
      }

      //      TODO: Max needs to operate on every 1d array within this ND view
      array_out[i] = fetch::math::statistics::Max(GetRange(arr_view));
    }

    return array_out;
  }

  type      Min() const { return this->super_type::Min(); }
  self_type Min(std::size_t fixed_axis) const
  {
    // set up the new output array
    std::vector<std::size_t> new_shape{shape()};
    new_shape.erase(new_shape.begin() + int(fixed_axis));
    NDArray array_out{new_shape};

    for (std::size_t i = 0; i < shape()[fixed_axis]; ++i)
    {

      // set up the view to extract
      NDArrayView arr_view;

      for (std::size_t j = 0; j < shape().size(); ++j)
      {
        if (j == fixed_axis)
        {
          arr_view.from.push_back(i);
          arr_view.to.push_back(i + 1);
          arr_view.step.push_back(1);
        }
        else
        {
          arr_view.from.push_back(0);
          arr_view.to.push_back(shape()[j]);
          arr_view.step.push_back(1);
        }
      }

      //      TODO: Min needs to operate on every 1d array within this ND view
      array_out[i] = fetch::math::statistics::Min(GetRange(arr_view));
    }

    return array_out;
  }

private:
  std::size_t ComputeRowIndex(std::vector<std::size_t> &indices) const
  {

    std::size_t index  = 0;
    std::size_t n_dims = indices.size();
    std::size_t base   = 1;

    // loop through all dimensions
    for (std::size_t i = n_dims - 1; i == 0; --i)
    {
      index += indices[i] * base;
      base *= shape_[i];
    }
    return index;
  }
  std::size_t ComputeColIndex(std::vector<std::size_t> &indices) const
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
