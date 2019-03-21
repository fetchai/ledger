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
#include "math/distance/conditional_probabilities.hpp"

#include <cmath>

namespace fetch {
namespace math {
namespace statistics {

template <typename ArrayType>
typename ArrayType::Type Entropy(ArrayType const &a, std::size_t index,
                                 typename ArrayType::Type &ret)
{
  ret = 0;
  for (std::size_t j = 0; j < a.shape().at(0); ++j)
  {
    typename ArrayType::Type tmp_val = distance::ConditionalProbabilitiesDistance(a, j, index, 1);
    tmp_val *= Log2(tmp_val);
    ret += tmp_val;
  }

  return -ret;
}

template <typename ArrayType>
typename ArrayType::Type Entropy(ArrayType const &a, std::size_t index)
{
  typename ArrayType::Type ret;
  ConditionalProbabilitiesDistance(a, index, ret);
  return ret;
}

}  // namespace statistics
}  // namespace math
}  // namespace fetch
