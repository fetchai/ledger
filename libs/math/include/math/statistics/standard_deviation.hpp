#pragma once
#include "core/assert.hpp"
#include "math/shape_less_array.hpp"
#include "math/statistics/variance.hpp"
#include "vectorise/memory/range.hpp"

#include <cmath>

namespace fetch {
namespace math {
namespace statistics {

template <typename A>
inline typename A::type StandardDeviation(A const &a)
{
  using type = typename A::type;

  type m = Variance(a);
  return std::sqrt(m);
}

}  // namespace statistics
}  // namespace math
}  // namespace fetch
