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
#include "math/linalg/matrix.hpp"
#include "math/ndarray.hpp"
#include "math/shape_less_array.hpp"
#include "vectorise/memory/range.hpp"

#include <limits>

#include <cmath>

namespace fetch {
namespace math {

template <typename T, typename C>
class NDArray;

namespace statistics {

namespace details {

template <typename T>
inline void MaxImplementation(T const &array, typename T::type &ret)
{
  using vector_register_type = typename T::vector_register_type;

  ret = array.data().in_parallel().Reduce(
      memory::TrivialRange(0, array.size()),
      [](vector_register_type const &a, vector_register_type const &b) { return max(a, b); });
}

template <typename T>
inline void MaxImplementation(T const &array, memory::Range r, typename T::type &ret)
{
  using vector_register_type = typename T::vector_register_type;

  if (r.is_trivial())
  {
    ret = array.data().in_parallel().Reduce(
        r, [](vector_register_type const &a, vector_register_type const &b) { return max(a, b); });
  }
  else
  {  // non-trivial range is not vectorised
    typename T::type ret = -std::numeric_limits<typename T::type>::max();
    for (auto i : array)
    {
      ret = std::max(ret, i);
    }
  }
}

}  // namespace details

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
 * @tparam T        data type
 * @tparam C        container type
 * @param array
 * @return
 */
template <typename T, typename C = memory::SharedArray<T>>
inline T Max(NDArray<T, C> const &array)
{
  T ret;
  details::MaxImplementation<NDArray<T, C>>(array, ret);
  return ret;
}
template <typename T, typename C = memory::SharedArray<T>>
inline T Max(linalg::Matrix<T, C> const &array)
{
  T ret;
  details::MaxImplementation<linalg::Matrix<T, C>>(array, ret);
  return ret;
}
template <typename T, typename C = memory::SharedArray<T>>
inline T Max(RectangularArray<T, C> const &array)
{
  T ret;
  details::MaxImplementation<RectangularArray<T, C>>(array, ret);
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
template <typename T, typename C = memory::SharedArray<T>>
inline T Max(NDArray<T, C> const &array, memory::Range r)
{
  T ret;
  details::MaxImplementation<NDArray<T, C>>(array, r, ret);
  return ret;
}
template <typename T, typename C = memory::SharedArray<T>>
inline T Max(linalg::Matrix<T, C> const &array, memory::Range r)
{
  T ret;
  details::MaxImplementation<linalg::Matrix<T, C>>(array, r, ret);
  return ret;
}
template <typename T, typename C = memory::SharedArray<T>>
inline T Max(RectangularArray<T, C> const &array, memory::Range r)
{
  T ret;
  details::MaxImplementation<RectangularArray<T, C>>(array, r, ret);
  return ret;
}

}  // namespace statistics
}  // namespace math
}  // namespace fetch
