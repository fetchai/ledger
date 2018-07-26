#pragma once
#include "core/assert.hpp"
#include "math/shape_less_array.hpp"
#include "vectorise/memory/range.hpp"

#include <cmath>

namespace fetch {
namespace math {
namespace distance {

template <typename T, std::size_t S = memory::VectorSlice<T>::E_TYPE_SIZE>
inline typename memory::VectorSlice<T, S>::type Braycurtis(
    memory::VectorSlice<T, S> const &a, memory::VectorSlice<T, S> const &b)
{
  detailed_assert(a.size() == b.size());
  typedef typename memory::VectorSlice<T, S>::vector_register_type
                                                   vector_register_type;
  typedef typename memory::VectorSlice<T, S>::type type;

  type sumA = a.in_parallel().SumReduce(
      memory::TrivialRange(0, a.size()),
      [](vector_register_type const &x, vector_register_type const &y) {
        return abs(x - y);
      },
      b);

  type sumB = a.in_parallel().SumReduce(
      memory::TrivialRange(0, b.size()),
      [](vector_register_type const &x, vector_register_type const &y) {
        return abs(x + y);
      },
      b);

  return type(sumA / sumB);
}

template <typename T, typename C>
inline typename ShapeLessArray<T, C>::type Braycurtis(
    ShapeLessArray<T, C> const &a, ShapeLessArray<T, C> const &b)
{
  return Braycurtis(a.data(), b.data());
}

}  // namespace distance
}  // namespace math
}  // namespace fetch

