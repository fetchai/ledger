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
#include "math/kernels/standard_functions.hpp"

#include <cassert>

namespace fetch {
namespace math {

/**
 * Cross entropy loss with x as the prediction, and y as the ground truth
 * @tparam ArrayType
 * @param x a 2d array with axis 0 = examples, and axis 1 = dimension in prediction space
 * @param y same size as x with the correct predictions set to 1 in axis 1 and all other positions =
 * 0
 * @return Returns an Array of size 1 containing the loss value
 */
template <typename ArrayType>
void SoftmaxCrossEntropyLoss(ArrayType const &x, ArrayType const &y, ArrayType &ret)
{
  assert(x.shape() == y.shape());
  assert(x.shape().size() == 2);

  auto n_examples = x.shape()[0];
  ArrayType sce_x = x.Clone();

  // we don't explicitly call softmax, because we assume softmax was already included in the graph
  // (i.e. x is the output of softmax layer)

  auto      gt = ArgMax(y);
  ArrayType log_likelihood{1};
  log_likelihood.Fill(0);

  for (typename ArrayType::SizeType idx = 0; idx < n_examples; ++idx)
  {
    log_likelihood.At(0) -= std::log(sce_x.At({idx, gt}));
  }
  Divide(log_likelihood, static_cast<typename ArrayType::Type>(n_examples), ret);
}

template <typename ArrayType>
ArrayType SoftmaxCrossEntropyLoss(ArrayType const &x, ArrayType const &y)
{
  ArrayType ret{x.shape()};
  SoftmaxCrossEntropyLoss(x, y, ret);
  return ret;
}

}  // namespace math
}  // namespace fetch
