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
#include "math/standard_functions/log.hpp"
#include <cassert>

namespace fetch {
namespace math {

/**
 * Kullback-Leibler divergence between matrix A and B
 * i.e. for each point j and each point i do Sum(CPD(j,i,A)*log(CPD(j,i,A)/CPD(j,i,B))) Where
 * CPD(j,i,n) means ConditionalProbabilitiesDistance of points j and i of matrix n, where sigma=1
 * @param A input tensor of dimensions n_data x n_features
 * @param B input tensor of dimensions n_data x n_features
 * @return
 */
template <typename ArrayType>
typename ArrayType::Type KlDivergence(ArrayType const &a, ArrayType const &b,
                                      typename ArrayType::Type &ret)
{
  using DataType = typename ArrayType::Type;
  assert(a.shape().at(0) == b.shape().at(0));

  ret = DataType(0);
  for (std::size_t i = 0; i < a.shape().at(0); i++)
  {
    for (std::size_t j = 0; j < a.shape().at(0); j++)
    {
      if (i == j)
        continue;

      DataType tmp_p_j_i(distance::ConditionalProbabilitiesDistance(a, i, j, DataType(1)));
      DataType tmp_q_j_i(distance::ConditionalProbabilitiesDistance(b, i, j, DataType(1)));

      ret += Multiply(tmp_p_j_i, Log(Divide(tmp_p_j_i, tmp_q_j_i)));
    }
  }

  return ret;
}

template <typename ArrayType>
typename ArrayType::Type KlDivergence(ArrayType const &a, ArrayType const &b)
{
  typename ArrayType::Type ret;
  KlDivergence(a, b, ret);
  return ret;
}

}  // namespace math
}  // namespace fetch