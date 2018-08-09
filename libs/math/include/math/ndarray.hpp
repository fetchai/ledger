#pragma once
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
  template <typename... Indices>
  type &operator()(Indices const &... indices)
  {
    assert(sizeof...(indices) <= shape_.size());
    std::size_t index = 0, shift = 1;
    ComputeIndex(0, index, shift, indices...);
    return this->operator[](index);
  }

  self_type operator()(NDArrayView array_view) const
  {
    std::vector<std::size_t> new_shape;

    // instantiate new array of the right shape!
    for (std::size_t cur_dim = 0; cur_dim < array_view.from.size(); ++cur_dim)
    {
      std::size_t cur_from = array_view.from[cur_dim];
      std::size_t cur_to   = array_view.to[cur_dim];
      std::size_t cur_step = array_view.step[cur_dim];
      std::size_t cur_len  = (cur_to - cur_from) / cur_step;
      new_shape.push_back(cur_len);
    }
    self_type output = self_type(new_shape);

    // populate new array with the correct data
    for (std::size_t cur_dim = 0; cur_dim < array_view.from.size(); ++cur_dim)
    {
      for (std::size_t cur_idx = 0; cur_idx < new_shape[cur_dim].size(); ++cur_idx)
      {
        cur_idx = ComputeIndex()

        output[]
        this->operator[]()

      }

      std::size_t cur_from = array_view.from[cur_dim];
      std::size_t cur_to   = array_view.to[cur_dim];
      std::size_t cur_step = array_view.step[cur_dim];
      std::size_t cur_len  = (cur_to - cur_from) / cur_step;
      new_shape.push_back(cur_len);
    }


    return output;
  }
  /**
   * A getter for accessing data in the array
   *
   * @param[out]     dest is the destination for the data to be copied.
   * @param[in]      indices specifies the data points to access.
   *
   **/
  template <typename D, typename... Indices>
  void Get(D &dest, Indices const &... indices)
  {
    std::size_t shift     = 1, size;
    int         dest_rank = int(shape_.size()) - int(sizeof...(indices));

    assert(dest_rank > 0);

    std::size_t rank_offset = (shape_.size() - std::size_t(dest_rank));

    for (std::size_t i = rank_offset; i < shape_.size(); ++i)
    {
      shift *= shape_[i];
    }

    size = shift;

    std::size_t offset = 0;
    ComputeIndex(0, offset, shift, indices...);

    dest.Resize(shape_, rank_offset);
    assert(size <= dest.size());

    for (std::size_t i = 0; i < size; ++i)
    {
      dest[i] = this->operator[](offset + i);
    }
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

private:
  template <typename... Indices>
  void ComputeIndex(std::size_t const &N, std::size_t &index, std::size_t &shift,
                    std::size_t const &next, Indices const &... indices) const
  {
    ComputeIndex(N + 1, index, shift, indices...);

    assert(N < shape_.size());
    index += next * shift;
    shift *= shape_[N];
  }

  template <typename... Indices>
  void ComputeIndex(std::size_t const &N, std::size_t &index, std::size_t &shift, int const &next,
                    Indices const &... indices) const
  {
    ComputeIndex(N + 1, index, shift, indices...);

    assert(N < shape_.size());
    index += std::size_t(next) * shift;
    shift *= shape_[N];
  }

  void ComputeIndex(std::size_t const &N, std::size_t &index, std::size_t &shift,
                    std::size_t const &next) const
  {
    index += next * shift;
    shift *= shape_[N];
  }

  void ComputeIndex(std::size_t const &N, std::size_t &index, std::size_t &shift,
                    int const &next) const
  {
    index += std::size_t(next) * shift;
    shift *= shape_[N];
  }

  std::size_t              size_ = 0;
  std::vector<std::size_t> shape_;
};
}  // namespace math
}  // namespace fetch
