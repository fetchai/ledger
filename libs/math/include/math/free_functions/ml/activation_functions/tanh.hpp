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
#include "math/free_functions/standard_functions/trigonometric.hpp"

namespace fetch {
namespace math {

/**
 * The tanh function
 * @tparam ArrayType
 * @tparam T
 * @param y
 * @param y_hat
 * @param ret
 */
template <typename ArrayType>
void TanhLayer(ArrayType const &t, ArrayType &ret)
{
    typename ArrayType::SizeType idx(0);
    for(auto &val: t){        
        Tanh(val, ret.At(idx));
        ++idx;
    };
    
}

template <typename ArrayType>
ArrayType TanhLayer(ArrayType const &t)
{
  ArrayType ret(t.shape());
  TanhLayer(t, ret);
  return ret;
}

}  // namespace math
}  // namespace fetch
