#pragma once
#include "math/shape_less_array.hpp"
#include "vectorise/memory/array.hpp"

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

  NDArray(std::size_t const &n = 0) : super_type(n) { size_ = 0; }
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

  /**
 * Provides an interfance to the l2loss function in the shape_less_array
 *
 * @return       returns a single value of type indicating the l2loss
 *
 **/
  type L2Loss()
  {
    return this->super_type::L2Loss();
  }

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
