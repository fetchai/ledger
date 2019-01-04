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
#include "math/shapeless_array.hpp"
#include "vectorise/memory/range.hpp"

#include <cmath>

namespace fetch {
namespace math {
namespace correlation {

template <typename T, std::size_t S = memory::VectorSlice<T>::E_TYPE_SIZE>
inline typename memory::VectorSlice<T, S>::Type Pearson(memory::VectorSlice<T, S> const &a,
                                                        memory::VectorSlice<T, S> const &b)
{
  detailed_assert(a.size() == b.size());
  using vector_register_type = typename memory::VectorSlice<T, S>::vector_register_type;
  using Type                 = typename memory::VectorSlice<T, S>::Type;

  Type meanA = a.in_parallel().Reduce(
      [](vector_register_type const &x, vector_register_type const &y) { return x + y; });

  Type meanB = b.in_parallel().Reduce(
      [](vector_register_type const &x, vector_register_type const &y) { return x + y; });

  meanA /= Type(a.size());
  meanB /= Type(b.size());

  vector_register_type mA(meanA);
  vector_register_type mB(meanB);

  Type innerA = a.in_parallel().SumReduce(memory::TrivialRange(0, a.size()),
                                          [mA](vector_register_type const &x) {
                                            vector_register_type d = x - mA;
                                            return d * d;
                                          });

  Type innerB = b.in_parallel().SumReduce(memory::TrivialRange(0, b.size()),
                                          [mB](vector_register_type const &x) {
                                            vector_register_type d = x - mB;
                                            return d * d;
                                          });

  Type top = a.in_parallel().SumReduce(
      memory::TrivialRange(0, a.size()),
      [mA, mB](vector_register_type const &x, vector_register_type const &y) {
        vector_register_type d1 = x - mA;
        vector_register_type d2 = y - mB;
        return d1 * d2;
      },
      b);

  Type denom = Type(sqrt(innerA * innerB));

  return Type(top / denom);
}

template <typename T, typename C>
inline typename ShapelessArray<T, C>::Type Pearson(ShapelessArray<T, C> const &a,
                                                   ShapelessArray<T, C> const &b)
{
  return Pearson(a.data(), b.data());
}

}  // namespace correlation
}  // namespace math
}  // namespace fetch
