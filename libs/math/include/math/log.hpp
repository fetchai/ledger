#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018 Fetch.AI Limited
//
//   Licensed under the Apache License, Version 2.0 (the "License");
//   you may not use this file except in compliance with the License.
//   You may obtain a copy of the License at
//
//       http://www.apache.org/licenses/LICENSE-2.0
//
//   Unless required by applicable law or agreed to in writing, software
//   distributed under the License is distributed on an "AS IS" BASIS,
//   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//   See the License for the specific language governing permissions and
//   limitations under the License.
//
//------------------------------------------------------------------------------
#include "core/assert.hpp"
#include "math/shape_less_array.hpp"
#include "vectorise/memory/range.hpp"

#include <cmath>

namespace fetch {
namespace math {

template <typename ARRAY_TYPE>
inline ARRAY_TYPE Log(ARRAY_TYPE const &array)
{
  //  using vector_register_type      = typename ARRAY_TYPE::vector_register_type;

  ARRAY_TYPE ret{array};
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

  ARRAY_TYPE ret{array};

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
