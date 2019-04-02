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

#include "math/fundamental_operators.hpp"  // add, subtract etc.
#include "math/standard_functions/exp.hpp"

namespace fetch {
namespace math {

/**
 * The sigmoid function - numerically stable
 * @tparam ArrayType
 * @tparam T
 * @param y
 * @param y_hat
 * @param ret
 */
template <typename ArrayType>
void Sigmoid(ArrayType const &t, ArrayType &ret)
{

  typename ArrayType::SizeType idx(0);
  for (auto &val : t)
  {
    if (val >= typename ArrayType::Type(0))
    {
      Multiply(typename ArrayType::Type(-1.0), val, ret.At(idx));
      Exp(ret.At(idx), ret.At(idx));
      Add(ret.At(idx), typename ArrayType::Type(1.0), ret.At(idx));
      Divide(typename ArrayType::Type(1.0), ret.At(idx), ret.At(idx));
    }
    else
    {
      Exp(val, ret.At(idx));
      Divide(ret.At(idx), ret.At(idx) + typename ArrayType::Type(1.0), ret.At(idx));
    }
    ++idx;
  }
}

template <typename ArrayType>
ArrayType Sigmoid(ArrayType const &t)
{
  ArrayType ret(t.shape());
  Sigmoid(t, ret);
  return ret;
}

}  // namespace math
}  // namespace fetch
