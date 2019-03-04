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

#include "math/free_functions/fundamental_operators.hpp"  // add, subtract etc.
#include "math/free_functions/standard_functions/exp.hpp"

namespace fetch {
namespace math {

/**
 * The sigmoid function
 * @tparam ArrayType
 * @tparam T
 * @param y
 * @param y_hat
 * @param ret
 */
template <typename ArrayType>
void Sigmoid(ArrayType const &t, ArrayType &ret)
{
  Multiply(typename ArrayType::Type(-1.0), ret, ret);
  Exp(ret);
  Add(ret, typename ArrayType::Type(1.0), ret);
  Divide(typename ArrayType::Type(1.0), ret, ret);
}

template <typename ArrayType>
ArrayType Sigmoid(ArrayType const &t)
{
  ArrayType ret;
  ret.Copy(t);
  Sigmoid(t, ret);
  return ret;
}

}  // namespace math
}  // namespace fetch
