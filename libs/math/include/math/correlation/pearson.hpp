#pragma once

#include "core/assert.hpp"
#include "math/shape_less_array.hpp"
#include "vectorise/memory/range.hpp"

#include <cmath>

namespace fetch {
namespace math {
namespace correlation {

template <typename T, std::size_t S = memory::VectorSlice<T>::E_TYPE_SIZE>
inline typename memory::VectorSlice<T, S>::type Pearson(memory::VectorSlice<T, S> const &a,
                                                        memory::VectorSlice<T, S> const &b)
{
  detailed_assert(a.size() == b.size());
  using vector_register_type = typename memory::VectorSlice<T, S>::vector_register_type;
  using type = typename memory::VectorSlice<T, S>::type;

  type meanA = a.in_parallel().Reduce(
      [](vector_register_type const &x, vector_register_type const &y) { return x + y; });

  type meanB = b.in_parallel().Reduce(
      [](vector_register_type const &x, vector_register_type const &y) { return x + y; });

  meanA /= type(a.size());
  meanB /= type(b.size());

  vector_register_type mA(meanA);
  vector_register_type mB(meanB);

  type innerA = a.in_parallel().SumReduce(memory::TrivialRange(0, a.size()),
                                          [mA](vector_register_type const &x) {
                                            vector_register_type d = x - mA;
                                            return d * d;
                                          });

  type innerB = b.in_parallel().SumReduce(memory::TrivialRange(0, b.size()),
                                          [mB](vector_register_type const &x) {
                                            vector_register_type d = x - mB;
                                            return d * d;
                                          });

  type top = a.in_parallel().SumReduce(
      memory::TrivialRange(0, a.size()),
      [mA, mB](vector_register_type const &x, vector_register_type const &y) {
        vector_register_type d1 = x - mA;
        vector_register_type d2 = y - mB;
        return d1 * d2;
      },
      b);

  type denom = type(sqrt(innerA * innerB));

  return type(top / denom);
}

template <typename T, typename C>
inline typename ShapeLessArray<T, C>::type Pearson(ShapeLessArray<T, C> const &a,
                                                   ShapeLessArray<T, C> const &b)
{
  return Pearson(a.data(), b.data());
}

}  // namespace correlation
}  // namespace math
}  // namespace fetch
