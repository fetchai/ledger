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

#include "math/free_functions/exponentiation/exponentiation.hpp"  // square
#include "math/free_functions/fundamental_operators.hpp"          // add, subtract etc.
#include "math/free_functions/matrix_operations/matrix_operations.hpp"
#include <cassert>

namespace fetch {
namespace math {

template <typename ArrayType>
typename ArrayType::Type MeanSquareError(ArrayType const &A, ArrayType const &B)
{
  assert(A.shape() == B.shape());
  ArrayType tmp_array(A.shape());

  Subtract(A, B, tmp_array);
  for (std::size_t j = 0; j < tmp_array.size(); ++j)
  {
    std::cout << "tmp_array[j]: " << tmp_array[j] << std::endl;
  }

  Square(tmp_array);

  for (std::size_t j = 0; j < tmp_array.size(); ++j)
  {
    std::cout << "tmp_array[j]: " << tmp_array[j] << std::endl;
  }

  typename ArrayType::Type ret = Sum(tmp_array);

  std::cout << "ret: " << ret << std::endl;
  for (std::size_t j = 0; j < tmp_array.size(); ++j)
  {
    std::cout << "tmp_array[j]: " << tmp_array[j] << std::endl;
  }

  ret = Divide(ret, typename ArrayType::Type(A.size()));

  std::cout << "A.size(): " << A.size() << std::endl;
  std::cout << "A.shape()[0]: " << A.shape()[0] << std::endl;

  std::cout << "ret: " << ret << std::endl;

  // TODO(private 343)
  // division by 2 allows us to cancel out with a 2 in the derivative
  ret = Divide(ret, typename ArrayType::Type(2));
  std::cout << "ret: " << ret << std::endl;

  return ret;
}

}  // namespace math
}  // namespace fetch
