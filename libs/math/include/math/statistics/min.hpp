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
inline void MinImplementation(T const &array, typename T::type &ret)
{
  using vector_register_type = typename T::vector_register_type;

  ret = array.data().in_parallel().Reduce(
      memory::TrivialRange(0, array.size()),
      [](vector_register_type const &a, vector_register_type const &b) { return min(a, b); });
}

template <typename T>
inline void MinImplementation(T const &array, memory::Range r, typename T::type &ret)
{
  using vector_register_type = typename T::vector_register_type;

  if (r.is_trivial())
  {
    ret = array.data().in_parallel().Reduce(
        r, [](vector_register_type const &a, vector_register_type const &b) { return min(a, b); });
  }
  else
  {  // non-trivial range is not vectorised
    typename T::type ret = std::numeric_limits<typename T::type>::max();
    for (auto i : array)
    {
      ret = std::min(ret, i);
    }
  }
}

}  // namespace details

/**
 * Min function for two values
 * @tparam T
 * @param datum1
 * @param datum2
 * @return
 */
template <typename T>
inline T Min(T const &datum1, T const &datum2)
{
  return std::min(datum1, datum2);
}

/**
 * Min function for array
 * @tparam ARRAY_TYPE
 * @param array
 * @return
 */
template <typename T, typename C = memory::SharedArray<T>>
inline T Min(NDArray<T, C> const &array)
{
  T ret;
  details::MinImplementation<NDArray<T, C>>(array, ret);
  return ret;
}
template <typename T, typename C = memory::SharedArray<T>>
inline T Min(linalg::Matrix<T, C> const &array)
{
  T ret;
  details::MinImplementation<linalg::Matrix<T, C>>(array, ret);
  return ret;
}
template <typename T, typename C = memory::SharedArray<T>>
inline T Min(RectangularArray<T, C> const &array)
{
  T ret;
  details::MinImplementation<RectangularArray<T, C>>(array, ret);
  return ret;
}

/**
 * Min function for applying max to a range within array
 * @tparam A
 * @tparam data_type
 * @param r
 * @param a
 * @return
 */
template <typename T, typename C = memory::SharedArray<T>>
inline T Min(NDArray<T, C> const &array, memory::Range r)
{
  T ret;
  details::MinImplementation<NDArray<T, C>>(array, r, ret);
  return ret;
}
template <typename T, typename C = memory::SharedArray<T>>
inline T Min(linalg::Matrix<T, C> const &array, memory::Range r)
{
  T ret;
  details::MinImplementation<linalg::Matrix<T, C>>(array, r, ret);
  return ret;
}
template <typename T, typename C = memory::SharedArray<T>>
inline T Min(RectangularArray<T, C> const &array, memory::Range r)
{
  T ret;
  details::MinImplementation<RectangularArray<T, C>>(array, r, ret);
  return ret;
}

}  // namespace statistics

}  // namespace math
}  // namespace fetch
