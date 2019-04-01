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

#include "math/fundamental_operators.hpp"   // divide
#include "math/matrix_operations.hpp"       //
#include "math/standard_functions/log.hpp"  // log
#include <cassert>

namespace fetch {
namespace math {

/**
 * Cross entropy loss with x as the prediction, and y as the ground truth
 * @tparam ArrayType
 * @param x a 2d array with axis 0 = examples, and axis 1 = dimension in prediction space
 * @param y same size as x with the correct predictions set to 1 in axis 1 and all other positions =
 * 0
 * @param n_classes if y is not a one-hot, then the number of classes should be 2, or otherwise
 * specified
 * @return
 */
template <typename ArrayType>
typename ArrayType::Type CrossEntropyLoss(
    ArrayType const &x, ArrayType const &y,
    typename ArrayType::SizeType n_classes = typename ArrayType::SizeType(2))
{
  using DataType = typename ArrayType::Type;
  using SizeType = typename ArrayType::SizeType;

  assert(x.shape() == y.shape());
  assert(x.shape().size() == 2);
  assert(n_classes > SizeType(1));

  auto n_examples = x.shape().at(0);
  auto n_dims     = x.shape().at(1);

  DataType ret = DataType(0);

  // if not a one-hot, must be binary logistic regression cost
  if (n_dims == 1)
  {
    assert(n_classes == SizeType(2));
    for (SizeType idx = 0; idx < n_examples; ++idx)
    {
      assert((y.At(idx) == DataType(1)) || (y.At(idx) == DataType(0)));
      if (y.At(idx) == DataType(1))
      {
        ret -= Log(x.At(idx));
      }
      else
      {
        DataType tmp = DataType(1) - x.At(idx);
        if (tmp <= 0)
        {
          throw std::runtime_error("cannot take log of negative values");
        }
        ret -= Log(tmp);
      }
    }
  }
  // if a one-hot, could be arbitrary n_classes
  else
  {
    ArrayType gt = ArgMax(y);  // y must be one hot - and we can ignore the zero cases

    for (SizeType idx = 0; idx < n_examples; ++idx)
    {
      ret -= Log(x.At({idx, SizeType(gt.At(idx))}));
    }
  }
  Divide(ret, static_cast<DataType>(n_examples), ret);
  return ret;
}
}  // namespace math
}  // namespace fetch
