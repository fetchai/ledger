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

#include <limits>

#include <cmath>

namespace fetch {
namespace math {
namespace statistics {

/**
 * Max function for two values
 * @tparam T
 * @param datum1
 * @param datum2
 * @return
 */
template <typename T>
inline T Max(T const &datum1, T const &datum2)
{
  return std::max(datum1, datum2);
}

/**
 * Max function for array
 * @tparam ARRAY_TYPE
 * @param array
 * @return
 */
template <typename ARRAY_TYPE>
inline typename ARRAY_TYPE::type Max(ARRAY_TYPE const &array)
{
  using vector_register_type = typename ARRAY_TYPE::vector_register_type;
  using data_type            = typename ARRAY_TYPE::type;

  data_type ret = array.data().in_parallel().Reduce(
      memory::TrivialRange(0, array.size()),
      [](vector_register_type const &a, vector_register_type const &b) { return max(a, b); });

  return ret;
}

/**
 * Max function for applying max to a range within array
 * @tparam A
 * @tparam data_type
 * @param r
 * @param a
 * @return
 */
template <typename ARRAY_TYPE>
inline typename ARRAY_TYPE::type Max(ARRAY_TYPE const &a, memory::Range r)
{
  using vector_register_type = typename ARRAY_TYPE::vector_register_type;
  using data_type            = typename ARRAY_TYPE::type;

  if (r.is_trivial())
  {
    data_type ret = a.data().in_parallel().Reduce(
        r, [](vector_register_type const &a, vector_register_type const &b) { return max(a, b); });

    return ret;
  }
  else
  {  // non-trivial range is not vectorised
    data_type ret = std::numeric_limits<data_type>::min();
    for (auto i : a)
    {
      ret = std::max(ret, i);
    }
  }
}

}  // namespace statistics
}  // namespace math
}  // namespace fetch
