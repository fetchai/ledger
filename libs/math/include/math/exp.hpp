#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018-2019 Fetch.AI Limited
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
#include "math/shapeless_array.hpp"
#include "vectorise/memory/range.hpp"

#include "math/linalg/matrix.hpp"
#include "math/ndarray.hpp"

#include <cmath>

namespace fetch {
namespace math {

namespace details {
template <typename T>
inline void ExpImplementation(T const &array, T &ret)
{
  ret.ResizeFromShape(array.shape());
  for (std::size_t i = 0; i < array.size(); ++i)
  {
    ret[i] = std::exp(array[i]);
  }
}
template <typename T>
inline void ExpImplementation(T const &array, memory::Range const &r, T &ret)
{
  ret.Reshape(array.shape());

  if (r.is_trivial())
  {
    for (std::size_t i = 0; i < array.size(); ++i)
    {
      ret[i] = std::exp(array[i]);
    }
  }
  else
  {  // non-trivial range is not vectorised
    for (std::size_t i = 0; i < array.size(); i += r.step())
    {
      ret[i] = std::exp(array[i]);
    }
  }
}
}  // namespace details

template <typename T, typename C = memory::SharedArray<T>>
inline void Exp(NDArray<T, C> const &array, NDArray<T, C> ret)
{
  details::ExpImplementation<NDArray<T, C>>(array, ret);
}
template <typename T, typename C = memory::SharedArray<T>>
inline void Exp(linalg::Matrix<T, C> const &array, linalg::Matrix<T, C> ret)
{
  details::ExpImplementation<linalg::Matrix<T, C>>(array, ret);
}
template <typename T, typename C = memory::SharedArray<T>>
inline void Exp(RectangularArray<T, C> const &array, RectangularArray<T, C> &ret)
{
  details::ExpImplementation<RectangularArray<T, C>>(array, ret);
}

/**
 * Applies Exp elementwise to all elements in specified range
 * @tparam ARRAY_TYPE   the type of array containing elements
 * @param r             the specified range
 * @param array         the specified array
 * @return
 */

template <typename T, typename C = memory::SharedArray<T>>
inline void Exp(NDArray<T, C> const &array, memory::Range r, NDArray<T, C> &ret)
{
  details::ExpImplementation<NDArray<T, C>>(array, r, ret);
}
template <typename T, typename C = memory::SharedArray<T>>
inline void Exp(linalg::Matrix<T, C> const &array, memory::Range r, linalg::Matrix<T, C> &ret)
{
  details::ExpImplementation<linalg::Matrix<T, C>>(array, r, ret);
}
template <typename T, typename C = memory::SharedArray<T>>
inline void Exp(RectangularArray<T, C> const &array, memory::Range r, RectangularArray<T, C> &ret)
{
  details::ExpImplementation<RectangularArray<T, C>>(array, r, ret);
}

}  // namespace math
}  // namespace fetch
