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

#include "math/fundamental_operators.hpp"    // add, subtract etc.
#include "math/standard_functions/pow.hpp"   // square
#include "math/standard_functions/sqrt.hpp"  // sqrt

namespace fetch {
namespace math {

template <typename ArrayType>
typename ArrayType::Type L2Norm(ArrayType const &A)
{
  ArrayType ret{A.shape()};
  return Sqrt(Sum(Square(A)));
}

}  // namespace math
}  // namespace fetch
