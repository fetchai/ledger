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

#include "math/distance/absolute.hpp"
#include "math/matrix_operations.hpp"

namespace fetch {
namespace math {

template <typename ArrayType>
typename ArrayType::Type MeanAbsoluteError(ArrayType const &A, ArrayType const &B)
{
  typename ArrayType::Type ret = distance::AbsoluteDistance(A, B);
  Divide(ret, static_cast<typename ArrayType::Type>(A.size()), ret);
  return ret;
}

}  // namespace math
}  // namespace fetch
