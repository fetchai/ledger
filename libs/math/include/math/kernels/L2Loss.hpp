#pragma once
#include "core/assert.hpp"
#include "math/shape_less_array.hpp"
#include "vectorise/memory/range.hpp"

#include <cmath>

namespace fetch {
namespace math {
namespace kernels {

template <typename T, std::size_t S = memory::VectorSlice<T>::E_TYPE_SIZE>
inline typename memory::VectorSlice<T, S>::type L2Loss(memory::VectorSlice<T, S> const &a,
                                                       memory::VectorSlice<T, S> const &b)
{
  detailed_assert(a.size() == b.size());
  using vector_register_type = typename memory::VectorSlice<T, S>::vector_register_type;
  using type                 = typename memory::VectorSlice<T, S>::type;

  type l2loss =
      a.in_parallel().SumReduce(memory::TrivialRange(0, a.size()),
                                [](vector_register_type const &x, vector_register_type const &y) {
                                  vector_register_type d = x - y;
                                  return d * d;
                                },
                                b);
  l2loss /= 2;
  return l2loss;
}

template <typename T, typename C>
inline typename ShapeLessArray<T, C>::type L2Loss(ShapeLessArray<T, C> const &a,
                                                  ShapeLessArray<T, C> const &b)
{
  return L2Loss(a.data(), b.data());
}

}  // namespace kernels
}  // namespace math
}  // namespace fetch
