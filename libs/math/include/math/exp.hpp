#pragma once
#include "core/assert.hpp"
#include "math/shape_less_array.hpp"
#include "vectorise/memory/range.hpp"

#include <cmath>

// TODO: implement vectorisation for exp
namespace fetch {
namespace math {

template <typename ARRAY_TYPE>
inline ARRAY_TYPE Exp(ARRAY_TYPE const &array)
{

  ARRAY_TYPE ret{array};
  for (std::size_t i = 0; i < array.size(); ++i)
  {
    ret[i] = std::exp(array[i]);
  }

  return ret;
}

/**
 * Applies Exp elementwise to all elements in specified range
 * @tparam ARRAY_TYPE   the type of array containing elements
 * @param r             the specified range
 * @param array         the specified array
 * @return
 */
template <typename ARRAY_TYPE>
inline ARRAY_TYPE Exp(ARRAY_TYPE const &array, memory::Range r)
{
  //  using vector_register_type = typename ARRAY_TYPE::vector_register_type;

  ARRAY_TYPE ret{array};

  if (r.is_trivial())
  {
    for (std::size_t i = 0; i < array.size(); ++i)
    {
      ret[i] = std::exp(array[i]);
    }
    //    ret = array.data().in_parallel().Apply(
    //        r,
    //        [](vector_register_type &a) { return std::exp(a); });
  }
  else
  {  // non-trivial range is not vectorised
    for (std::size_t i = 0; i < array.size(); ++i)
    {
      ret[i] = std::exp(array[i]);
    }
  }
  return ret;
}

}  // namespace math
}  // namespace fetch
