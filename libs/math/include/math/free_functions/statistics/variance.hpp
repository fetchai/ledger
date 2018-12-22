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
#include "math/free_functions/statistics/mean.hpp"
#include "math/shapeless_array.hpp"
#include "vectorise/memory/range.hpp"

#include <cmath>

namespace fetch {
namespace math {
namespace statistics {

template <typename A>
inline typename A::Type Variance(A const &a)
{
  using vector_register_type = typename A::vector_register_type;
  using type                 = typename A::Type;

  type                 m = Mean(a);
  vector_register_type mean(m);

  type ret = a.data().in_parallel().SumReduce(memory::TrivialRange(0, a.size()),
                                              [mean](vector_register_type const &x) {
                                                vector_register_type d = x - mean;
                                                return d * d;
                                              });

  ret /= static_cast<type>(a.size());

  return ret;
}

}  // namespace statistics
}  // namespace math
}  // namespace fetch
