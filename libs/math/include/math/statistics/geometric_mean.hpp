#pragma once
#include "core/assert.hpp"
#include "math/shape_less_array.hpp"
#include "vectorise/memory/range.hpp"

#include <cmath>

namespace fetch {
namespace math {
namespace statistics {

template <typename A>
inline typename A::type GeometricMean(A const &a)
{
  typedef typename A::vector_register_type vector_register_type;
  typedef typename A::type                 type;

  type ret = a.data().in_parallel().Reduce(
      memory::TrivialRange(0, a.size()),
      [](vector_register_type const &a, vector_register_type const &b) {
        return a * b;
      });

  return type(std::pow(ret, double(1.) / double(a.size())));
}

}  // namespace statistics
}  // namespace math
}  // namespace fetch
