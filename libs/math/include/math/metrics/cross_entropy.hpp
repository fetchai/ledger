#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018-2020 Fetch.AI Limited
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

#include "math/fundamental_operators.hpp"
#include "math/matrix_operations.hpp"
#include "math/standard_functions/log.hpp"

#include <cassert>
#include <stdexcept>

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
typename ArrayType::Type CrossEntropyLoss(ArrayType const &x, ArrayType const &y)
{
  using DataType = typename ArrayType::Type;

  assert(x.shape() == y.shape());
  assert(x.shape().size() == 2);

  auto n_examples = x.shape().at(1);
  auto n_dims     = x.shape().at(0);

  auto ret = DataType{0};

  // if not a one-hot, must be binary logistic regression cost
  if (n_dims == 1)
  {
    auto     x_it = x.cbegin();
    auto     y_it = y.cbegin();
    DataType tmp;

    while (x_it.is_valid())
    {
      assert((*y_it == DataType{1}) || (*y_it == DataType{0}));
      if (*y_it == DataType{1})
      {
        ret = static_cast<DataType>(ret - Log(*x_it));
      }
      else
      {
        tmp = static_cast<DataType>(DataType{1} - *x_it);
        if (tmp <= DataType{0})
        {
          throw exceptions::NegativeLog("cannot take log of negative values");
        }
        ret = static_cast<DataType>(ret - Log(tmp));
      }
      ++x_it;
      ++y_it;
    }
  }
  // if a one-hot, could be arbitrary n_classes
  else
  {
    ArrayType gt = ArgMax(y, 0);  // y must be one hot - and we can ignore the zero cases

    for (SizeType idx = 0; idx < n_examples; ++idx)
    {
      ret = static_cast<DataType>(ret - Log(x.At(SizeType(gt[idx]), idx)));
    }
  }
  Divide(ret, static_cast<DataType>(n_examples), ret);
  return ret;
}
}  // namespace math
}  // namespace fetch
