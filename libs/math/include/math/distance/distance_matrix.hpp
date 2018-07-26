#pragma once
#include "core/assert.hpp"
#include "math/shape_less_array.hpp"

#include <cmath>

namespace fetch {
namespace math {
namespace distance {

template <typename A, typename F>
inline A &DistanceMatrix(A &r, A const &a, A const &b, F &&metric)
{
  detailed_assert(r.height() == a.height());
  detailed_assert(r.width() == b.height());

  std::size_t offset1 = 0;
  for (std::size_t i = 0; i < r.height(); ++i)
  {
    typename A::vector_slice_type slice1 = a.data().slice(offset1, a.width());
    offset1 += a.padded_width();
    std::size_t offset2 = 0;

    for (std::size_t j = 0; j < r.width(); ++j)
    {
      typename A::vector_slice_type slice2 = b.data().slice(offset2, b.width());
      offset2 += b.padded_width();
      r(i, j) = metric(slice1, slice2);
    }
  }
  return r;
}

}  // namespace distance
}  // namespace math
}  // namespace fetch

