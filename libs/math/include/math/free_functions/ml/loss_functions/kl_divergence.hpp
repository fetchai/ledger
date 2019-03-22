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

#include "math/distance/conditional_probabilities.hpp"
#include <cassert>

namespace fetch {
namespace math {

template <typename ArrayType>
typename ArrayType::Type KlDivergence(ArrayType const &A, ArrayType const &B)
{
  assert(A.shape().at(0) == B.shape().at(0));

  typename ArrayType::Type ret = 0;
  for (std::size_t i = 0; i < A.shape().at(0); i++)
    for (std::size_t j = 0; j < A.shape().at(0); j++)
    {
      {
        if (i == j)
          continue;

        typename ArrayType::Type tmp_p_j_i;
        distance::ConditionalProbabilitiesDistance(A, i, j, 1, tmp_p_j_i);
        typename ArrayType::Type tmp_q_j_i;
        distance::ConditionalProbabilitiesDistance(B, i, j, 1, tmp_q_j_i);

        ret += tmp_p_j_i * log10(Divide(tmp_p_j_i, tmp_q_j_i));
      }
    }

  return ret;
}

}  // namespace math
}  // namespace fetch