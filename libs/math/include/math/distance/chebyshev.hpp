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
namespace distance {

template <typename A>
inline typename A::Type Chebyshev(A const &a, A const &b)
{
  detailed_assert(a.size() == b.size());
  //  using vector_register_type = typename A::vector_register_type;
  //  using Type = typename A::Type;

  throw std::runtime_error("not implemented yet due to lacking features in vectorisation unit");

  /*
  Type m = a.data().in_parallel().Reduce(memory::TrivialRange(0, a.size()),
  [](vector_register_type const &x, vector_register_type const &y) { return
  max(x,y);
    }, b.data());
  */
  //  return m;m
  return 0;
}

}  // namespace distance
}  // namespace math
}  // namespace fetch
