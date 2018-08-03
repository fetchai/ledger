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

  /*
   * Returns a copy of the object
   */
  self_type Copy()
  {
      self_type copy = self_type(super_type::Copy());
      copy.LazyReshape(this->shape());
      return copy;
  }

  /*
   * flattens to 1 dimension efficiently
   */
  void Flatten()
  {
    shape_.clear();
    shape_.push_back(super_type::size());
  }

  /*
   * directly copies shape value without checking anything
   */
  void LazyReshape(std::vector<std::size_t> const &shape) { shape_ = shape; }

  template <typename... Indices>
  type &operator()(Indices const &... indices)
  {
    assert(sizeof...(indices) <= shape_.size());
    std::size_t index = 0, shift = 1;
    ComputeIndex(0, index, shift, indices...);
    return this->operator[](index);
  }

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

  /*
   * reshapes the array as specified.
   * returns false if new shape does not match number of elements in array.
   */
  bool Reshape(std::vector<std::size_t> const &shape)
  {
      shape_.clear();
      std::size_t total = 1;
      for (auto const &s : shape)
      {
          total *= s;
          shape_.push_back(s);
      }
      auto success = false;
      (total != this->size()) ? success = false : success = true;
      return success;
  }

  /*
   * returns the shape of the array
   */
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
