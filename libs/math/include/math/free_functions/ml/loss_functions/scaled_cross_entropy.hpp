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

#include "math/free_functions/exponentiation/exponentiation.hpp"
#include "math/free_functions/fundamental_operators.hpp"  // add, subtract etc.
#include "math/free_functions/matrix_operations/matrix_operations.hpp"
#include "math/free_functions/ml/loss_functions/softmax_cross_entropy.hpp"
#include <cassert>

namespace fetch {
namespace math {

/**
 * Cross entropy loss with x as the prediction, and y as the ground truth
 * @tparam ArrayType
 * @param x a 2d array with axis 0 = examples, and axis 1 = dimension in prediction space
 * @param y same size as x with the correct predictions set to 1 in axis 1 and all other positions =
 * 0
 * @return
 */
template <typename ArrayType>
typename ArrayType::Type ScaledCrossEntropyLoss(ArrayType const &x, ArrayType const &y,
                                                ArrayType const &scalar)
{
  assert(x.shape() == y.shape());

  typename ArrayType::Type ret = typename ArrayType::Type(0);
  ArrayType                tmp{scalar.shape()};

  SoftmaxCrossEntropyLoss(x, y, tmp);
  Divide(tmp, scalar, tmp);
  Sum(tmp, ret);
  return ret;
}

}  // namespace math
}  // namespace fetch
