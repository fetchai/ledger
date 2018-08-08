#pragma once
#include "core/assert.hpp"
#include "math/shape_less_array.hpp"
#include "vectorise/memory/range.hpp"

#include <cmath>

namespace fetch {
namespace math {
namespace distance {

template <typename A>
inline typename A::type Chebyshev(A const &a, A const &b)
{
  detailed_assert(a.size() == b.size());
  //  using vector_register_type = typename A::vector_register_type;
  //  using type = typename A::type;

  throw std::runtime_error("not implemented yet due to lacking features in vectorisation unit");

  /*
  type m = a.data().in_parallel().Reduce(memory::TrivialRange(0, a.size()),
  [](vector_register_type const &x, vector_register_type const &y) { return
  max(x,y);
    }, b.data());
  */
  //  return m;m
  return 0;
}

}  // namespace distance
}  // namespace math
}  // namespace fetch
