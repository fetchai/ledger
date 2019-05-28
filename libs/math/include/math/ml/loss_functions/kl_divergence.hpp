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
#include "math/fundamental_operators.hpp"
#include "math/matrix_operations.hpp"
#include "math/meta/math_type_traits.hpp"
#include "math/standard_functions/log.hpp"

namespace fetch {
namespace math {

/**
 * Kullback-Leibler divergence between array P and Q
 * KL(P||Q) = P log (P / Q)
 * * i.e. for each val in p and q do sum(p*log(p/q))
 * @param q input array of probabilities
 * @param p input array of probabilities
 * @return
 */
template <typename ArrayType>
meta::IfIsMathArray<ArrayType, void> KlDivergence(ArrayType const &p, ArrayType const &q,
                                                  typename ArrayType::Type &ret)
{
  ASSERT(p.shape().at(0) == q.shape().at(0));
  ret = Sum(Multiply(p, Log(Divide(p, q))));
}

template <typename ArrayType>
meta::IfIsMathArray<ArrayType, typename ArrayType::Type> KlDivergence(ArrayType const &p,
                                                                      ArrayType const &q)
{
  typename ArrayType::Type ret;
  KlDivergence(p, q, ret);
  return ret;
}

}  // namespace math
}  // namespace fetch