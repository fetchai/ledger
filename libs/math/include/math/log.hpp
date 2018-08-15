#pragma once
#include "core/assert.hpp"
#include "math/shape_less_array.hpp"
#include "vectorise/memory/range.hpp"

#include <cmath>

// TODO: implement vectorisation for log
namespace fetch {
namespace math {

template <typename ARRAY_TYPE>
inline ARRAY_TYPE Log(ARRAY_TYPE const &array)
{
  //  using vector_register_type      = typename ARRAY_TYPE::vector_register_type;

  ARRAY_TYPE ret{array.shape()};
  for (std::size_t i = 0; i < array.size(); ++i)
  {
    ret[i] = std::log(array[i]);
  }
  //  ret = array.data().in_parallel().Apply(
  //      memory::TrivialRange(0, array.size()),
  //      [](vector_register_type &a) { return std::log(a); });

  return ret;
}

/**
 * Applies Log elementwise to all elements in specified range
 * @tparam ARRAY_TYPE   the type of array containing elements
 * @param r             the specified range
 * @param array         the specified array
 * @return
 */
template <typename ARRAY_TYPE>
inline ARRAY_TYPE Log(ARRAY_TYPE const &array, memory::Range r)
{
  //  using vector_register_type = typename ARRAY_TYPE::vector_register_type;

  ARRAY_TYPE ret{array.shape()};

  if (r.is_trivial())
  {
    for (std::size_t i = 0; i < array.size(); ++i)
    {
      ret[i] = std::log(array[i]);
    }
    //    ret = array.data().in_parallel().Apply(
    //        r,
    //        [](vector_register_type &a) { return std::log(a); });
  }
  else
  {  // non-trivial range is not vectorised
    for (std::size_t i = 0; i < array.size(); ++i)
    {
      ret[i] = std::log(array[i]);
    }
  }
  return ret;
}

}  // namespace math
}  // namespace fetch
