#pragma once
#include "core/assert.hpp"
#include "math/shape_less_array.hpp"
#include "vectorise/memory/range.hpp"

#include <limits>

#include <cmath>

namespace fetch {
namespace math {
namespace statistics {

template <typename ARRAY_TYPE>
inline typename ARRAY_TYPE::type Min(ARRAY_TYPE const &array)
{
  using vector_register_type = typename ARRAY_TYPE::vector_register_type;
  using data_type            = typename ARRAY_TYPE::type;

  data_type ret = array.data().in_parallel().Reduce(
      memory::TrivialRange(0, array.size()),
      [](vector_register_type const &a, vector_register_type const &b) { return min(a, b); });

  return ret;
}

/**
 * This version of Min handles ranges and is important for both 1-d and n-dimensional arrays sinces
 * @tparam A
 * @tparam data_type
 * @param r
 * @param a
 * @return
 */
template <typename ARRAY_TYPE>
inline typename ARRAY_TYPE::type Min(ARRAY_TYPE const &a, memory::Range r)
{
  using vector_register_type = typename ARRAY_TYPE::vector_register_type;
  using data_type            = typename ARRAY_TYPE::type;

  if (r.is_trivial())
  {
    data_type ret = a.data().in_parallel().Reduce(
        r, [](vector_register_type const &a, vector_register_type const &b) { return min(a, b); });

    return ret;
  }
  else
  {  // non-trivial range is not vectorised
    data_type ret = std::numeric_limits<data_type>::max();
    for (auto i : a)
    {
      ret = std::min(ret, i);
    }
  }
}

}  // namespace statistics
}  // namespace math
}  // namespace fetch
