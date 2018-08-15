#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018 Fetch AI
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
namespace correlation {

template <typename T, std::size_t S = memory::VectorSlice<T>::E_TYPE_SIZE>
inline typename memory::VectorSlice<T, S>::type Jaccard(memory::VectorSlice<T, S> const &a,
                                                        memory::VectorSlice<T, S> const &b)
{
  detailed_assert(a.size() == b.size());
  using vector_register_type = typename memory::VectorSlice<T, S>::vector_register_type;
  using type                 = typename memory::VectorSlice<T, S>::type;

  vector_register_type zero(type(0));

  type sumA = a.in_parallel().SumReduce(
      memory::TrivialRange(0, a.size()),
      [zero](vector_register_type const &x, vector_register_type const &y) {
        return min(x != zero, y != zero);
      },
      b);

  type sumB = a.in_parallel().SumReduce(
      memory::TrivialRange(0, a.size()),
      [zero](vector_register_type const &x, vector_register_type const &y) {
        return max(x != zero, y != zero);
      },
      b);

  return type(sumA / (sumB));
}

template <typename T, typename C>
inline typename ShapeLessArray<T, C>::type Jaccard(ShapeLessArray<T, C> const &a,
                                                   ShapeLessArray<T, C> const &b)
{
  return Jaccard(a.data(), b.data());
}

template <typename T, std::size_t S = memory::VectorSlice<T>::E_TYPE_SIZE>
inline typename memory::VectorSlice<T, S>::type GeneralisedJaccard(
    memory::VectorSlice<T, S> const &a, memory::VectorSlice<T, S> const &b)
{
  detailed_assert(a.size() == b.size());
  using vector_register_type = typename memory::VectorSlice<T, S>::vector_register_type;
  using type                 = typename memory::VectorSlice<T, S>::type;

  type sumA = a.in_parallel().SumReduce(
      memory::TrivialRange(0, a.size()),
      [](vector_register_type const &x, vector_register_type const &y) { return min(x, y); }, b);

  type sumB = a.in_parallel().SumReduce(
      memory::TrivialRange(0, a.size()),
      [](vector_register_type const &x, vector_register_type const &y) { return max(x, y); }, b);

  return type(sumA / sumB);
}

template <typename T, typename C>
inline typename ShapeLessArray<T, C>::type GeneralisedJaccard(ShapeLessArray<T, C> const &a,
                                                              ShapeLessArray<T, C> const &b)
{
  return GeneralisedJaccard(a.data(), b.data());
}

}  // namespace correlation
}  // namespace math
}  // namespace fetch
