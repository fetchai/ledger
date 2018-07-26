#pragma once
#include "core/assert.hpp"
#include "math/shape_less_array.hpp"
#include "vectorise/memory/range.hpp"

#include <cmath>

namespace fetch {
namespace math {
namespace distance {

template <typename T, std::size_t S = memory::VectorSlice<T>::E_TYPE_SIZE>
inline typename memory::VectorSlice<T, S>::type Minkowski(
    memory::VectorSlice<T, S> const &a, memory::VectorSlice<T, S> const &b)
{
  detailed_assert(a.size() == b.size());
  typedef typename memory::VectorSlice<T, S>::type type;
  typedef typename memory::VectorSlice<T, S>::vector_register_type
      vector_register_type;

  type dist = a.data().in_parallel().SumReduce(
      memory::TrivialRange(0, a.size()),
      [n](vector_register_type const &x, vector_register_type const &y) {
        return pow(max(x, y) - min(x, y), n);
      },
      b.data());

  return std::pow(dist, 1. / double(n));
}

template <typename T, typename C>
inline typename ShapeLessArray<T, C>::type Minkowski(
    ShapeLessArray<T, C> const &a, ShapeLessArray<T, C> const &b)
{
  return Minkowski(a.data(), b.data());
}

}  // namespace distance
}  // namespace math
}  // namespace fetch

