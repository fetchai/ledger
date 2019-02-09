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
#include "math/kernels/standard_functions.hpp"
#include <cassert>

namespace fetch {
namespace math {

template <typename T, typename C>
class ShapelessArray;
template <typename T, typename C>
class NDArray;
template <typename T, typename C>
class NDArrayIterator;

template <typename T, typename C>
T Max(ShapelessArray<T, C> const &array);

template <typename ArrayType>
typename ArrayType::Type L2Norm(ArrayType const &A, ArrayType &ret)
{
  assert(A.size() == ret.size());
  assert(A.shape() == ret.shape());

  Square(A, ret);
  return std::sqrt(Sum(ret));
}
template <typename ArrayType>
typename ArrayType::Type L2Norm(ArrayType const &A)
{
  ArrayType ret{A.shape()};
  return L2Norm(A, ret);
}

template <typename ArrayType>
ArrayType MeanSquareError(ArrayType const &A, ArrayType const &B)
{
  assert(A.shape() == B.shape());
  ArrayType ret(A.shape());
  Subtract(A, B, ret);
  Square(ret);
  ret = ReduceSum(ret, 0);

  ret = Divide(ret, typename ArrayType::Type(A.shape()[0]));
  // TODO(private 343)
  ret = Divide(ret, typename ArrayType::Type(
                        2));  // division by 2 allows us to cancel out with a 2 in the derivative
  return ret;
}

/**
 * Cross entropy loss with x as the prediction, and y as the ground truth
 * @tparam ArrayType
 * @param x a 2d array with axis 0 = examples, and axis 1 = dimension in prediction space
 * @param y same size as x with the correct predictions set to 1 in axis 1 and all other positions =
 * 0
 * @return
 */
template <typename ArrayType>
ArrayType CrossEntropyLoss(ArrayType const &x, ArrayType const &y)
{
  assert(x.shape() == y.shape());

  // we can't handle taking log(0), and the user should ensure this is never asked for
  // if in doubt the user can always call SoftmaxCrossEntropyLoss instead
  for (std::size_t k = 0; k < x.size(); ++k)
  {
    assert(x.At(k) != 0);
  }

  ArrayType logx{x.shape()};
  logx.Copy(x);
  Log(logx);

  ArrayType plogx{logx.shape()};
  for (std::size_t i = 0; i < logx.shape()[0]; ++i)
  {
    for (std::size_t j = 0; j < logx.shape()[1]; ++j)
    {
      if (y.At(i, j) == 0)
      {
        plogx.Set(i, j, 0);
      }
      else if (logx.At(i, j) == 0)
      {
        plogx.Set(i, j, 0);
      }
      else
      {
        plogx.Set(i, j, logx.At(i, j) * y.At(i, j));
      }
    }
  }

  auto                     cel      = Multiply(plogx, -1.0);
  typename ArrayType::Type n        = typename ArrayType::Type(cel.shape()[0]);
  auto                     mean_cel = ReduceSum(cel, 0);

  return Divide(mean_cel, n);
}

/**
 * Cross entropy loss with x as the prediction, and y as the ground truth
 * @tparam ArrayType
 * @param x a 2d array with axis 0 = examples, and axis 1 = dimension in prediction space
 * @param y same size as x with the correct predictions set to 1 in axis 1 and all other positions =
 * 0
 * @return Returns an Array of size 1 containing the loss value
 */
template <typename ArrayType>
ArrayType SoftmaxCrossEntropyLoss(ArrayType const &x, ArrayType const &y)
{
  assert(x.shape() == y.shape());
  assert(x.shape().size() == 2);

  auto n_examples = x.shape()[0];

  ArrayType sce_x{x.shape()};
  sce_x.Copy(x);

  // we don't explicitly call softmax, because we assume softmax was already included in the graph
  // (i.e. x is the output
  //  of softmax layer)

  auto      gt = ArgMax(y, 1);
  ArrayType log_likelihood{1};
  log_likelihood[0] = 0;

  for (std::size_t idx = 0; idx < n_examples; ++idx)
  {
    sce_x.Set(idx, static_cast<std::size_t>(gt[idx]),
              std::log(sce_x.At(idx, static_cast<std::size_t>(gt[idx]))));
    log_likelihood[0] -= sce_x.At(idx, static_cast<std::size_t>(gt[idx]));
  }

  return Divide(log_likelihood, static_cast<typename ArrayType::Type>(n_examples));
}

}  // namespace math
}  // namespace fetch
