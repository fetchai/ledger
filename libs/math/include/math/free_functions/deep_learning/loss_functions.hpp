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

#include <cassert>

#include "math/free_functions/exponentiation/exponentiation.hpp"
#include "math/free_functions/fundamental_operators.hpp"  // add, subtract etc.
#include "math/kernels/standard_functions.hpp"

namespace fetch {
namespace math {

template <typename T, typename C>
class ShapelessArray;
template <typename T, typename C>
class NDArray;
template <typename T, typename C>
class NDArrayIterator;

namespace linalg {
template <typename T, typename C, typename S>
class Matrix;
}

template <typename T, typename C>
T Max(ShapelessArray<T, C> const &array);

template <typename T, typename C, typename S>
void ReduceSum(linalg::Matrix<T, C, S> const &obj1, std::size_t axis, linalg::Matrix<T, C, S> &ret)
{
  assert((axis == 0) || (axis == 1));
  std::vector<std::size_t> access_idx{0, 0};
  if (axis == 0)
  {
    assert(ret.size() == obj1.width());
    for (std::size_t i = 0; i < ret.size(); ++i)
    {
      ret[i] = 0;
      for (std::size_t j = 0; j < obj1.shape()[0]; ++j)
      {
        ret[i] += obj1(j, i);
      }
    }
  }
  else
  {
    assert(ret.size() == obj1.height());
    for (std::size_t i = 0; i < ret.size(); ++i)
    {
      ret[i] = 0;
      for (std::size_t j = 0; j < obj1.shape()[1]; ++j)
      {
        ret[i] += obj1(i, j);
      }
    }
  }
}
template <typename T, typename C, typename S>
linalg::Matrix<T, C, S> ReduceSum(linalg::Matrix<T, C, S> const &obj1,
                                  linalg::Matrix<T, C, S> const &axis)
{
  assert(axis.shape()[0] == 1);
  assert(axis.shape()[1] == 1);
  return ReduceSum(obj1, std::size_t(axis[0]));
}
template <typename T, typename C, typename S>
linalg::Matrix<T, C, S> ReduceSum(linalg::Matrix<T, C, S> const &obj1, std::size_t axis)
{
  assert((axis == 0) || (axis == 1));
  if (axis == 0)
  {
    std::vector<std::size_t> new_shape{1, obj1.width()};
    linalg::Matrix<T, C, S>  ret{new_shape};
    ReduceSum(obj1, axis, ret);
    return ret;
  }
  else
  {
    std::vector<std::size_t> new_shape{obj1.height(), 1};
    linalg::Matrix<T, C, S>  ret{new_shape};
    ReduceSum(obj1, axis, ret);
    return ret;
  }
}

template <typename T, typename C, typename S>
linalg::Matrix<T, C, S> ReduceSumImpl(linalg::Matrix<T, C, S> const &obj1, std::size_t const &axis)
{
  if (obj1.shape()[0] == 1)
  {
    return obj1;
  }
  else
  {
    return ReduceSumImpl(ReduceSum(obj1, axis), axis - 1);
  }
}
template <typename T, typename C, typename S>
linalg::Matrix<T, C, S> ReduceSum(linalg::Matrix<T, C, S> const &obj1)
{
  std::size_t axis = obj1.shape().size() - 1;

  return ReduceSumImpl(obj1, axis);
}

template <typename T, typename C, typename S>
linalg::Matrix<T, C, S> ReduceMean(linalg::Matrix<T, C, S> const &obj1, std::size_t const &axis)
{
  assert(axis == 0 || axis == 1);
  T n;
  if (axis == 0)
  {
    n = obj1.shape()[1];
  }
  else
  {
    n = obj1.shape()[0];
  }
  return Divide(ReduceSum(obj1, axis), n);
}

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
