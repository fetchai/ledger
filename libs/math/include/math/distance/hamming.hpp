#pragma once
#include "core/assert.hpp"
#include "math/shape_less_array.hpp"
#include "vectorise/memory/range.hpp"

namespace fetch {
namespace math {
namespace distance {

template <typename T, std::size_t S = memory::VectorSlice<T>::E_TYPE_SIZE>
inline typename memory::VectorSlice<T, S>::type Hamming(
    memory::VectorSlice<T, S> const &a, memory::VectorSlice<T, S> const &b)
{
  detailed_assert(a.size() == b.size());
  typedef typename memory::VectorSlice<T, S>::type type;
  typedef typename memory::VectorSlice<T, S>::vector_register_type
      vector_register_type;

  double dist = a.in_parallel().SumReduce(
      memory::TrivialRange(0, a.size()),
      [](vector_register_type const &x, vector_register_type const &y) {
        return x != y;
      },
      b);

  return type(dist) / type(a.size());
}

template <typename T, typename C>
inline typename ShapeLessArray<T, C>::type Hamming(
    ShapeLessArray<T, C> const &a, ShapeLessArray<T, C> const &b)
{
  return Hamming(a.data(), b.data());
}

}  // namespace distance
}  // namespace math
}  // namespace fetch
