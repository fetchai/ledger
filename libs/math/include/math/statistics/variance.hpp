#pragma once
#include "core/assert.hpp"
#include "math/shape_less_array.hpp"
#include "math/statistics/mean.hpp"
#include "vectorise/memory/range.hpp"

#include <cmath>

namespace fetch {
namespace math {
namespace statistics {

template <typename A>
inline typename A::type Variance(A const &a)
{
  using vector_register_type = typename A::vector_register_type;
  using type = typename A::type;

  type                 m = Mean(a);
  vector_register_type mean(m);

  type ret = a.data().in_parallel().SumReduce(memory::TrivialRange(0, a.size()),
                                              [mean](vector_register_type const &x) {
                                                vector_register_type d = x - mean;
                                                return d * d;
                                              });

  ret /= a.size();

  return ret;
}

}  // namespace statistics
}  // namespace math
}  // namespace fetch
