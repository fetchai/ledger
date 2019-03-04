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

//#include "math/free_functions/exponentiation/exponentiation.hpp"
#include "math/free_functions/fundamental_operators.hpp"  // add, subtract etc.
#include "math/kernels/standard_functions.hpp"            // square
#include <cassert>

namespace fetch {
namespace math {

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
  // division by 2 allows us to cancel out with a 2 in the derivative
  ret = Divide(ret, typename ArrayType::Type(2));
  return ret;
}

}  // namespace math
}  // namespace fetch
