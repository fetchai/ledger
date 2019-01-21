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

#include <cmath>

namespace fetch {
namespace math {
namespace kernels {

template <typename T, std::size_t S = memory::VectorSlice<T>::E_TYPE_SIZE>
inline typename memory::VectorSlice<T, S>::type L2Loss(memory::VectorSlice<T, S> const &a,
                                                       memory::VectorSlice<T, S> const &b)
{
  detailed_assert(a.size() == b.size());
  using vector_register_type = typename memory::VectorSlice<T, S>::vector_register_type;
  using type                 = typename memory::VectorSlice<T, S>::type;

  type l2loss =
      a.in_parallel().SumReduce(memory::TrivialRange(0, a.size()),
                                [](vector_register_type const &x, vector_register_type const &y) {
                                  vector_register_type d = x - y;
                                  return d * d;
                                },
                                b);
  l2loss /= 2;
  return l2loss;
}

template <typename T, typename C>
inline typename ShapelessArray<T, C>::type L2Loss(ShapelessArray<T, C> const &a,
                                                  ShapelessArray<T, C> const &b)
{
  return L2Loss(a.data(), b.data());
}

}  // namespace kernels
}  // namespace math
}  // namespace fetch
