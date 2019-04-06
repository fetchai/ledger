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

//#include "math/kernels/standard_functions.hpp"
#include "math/meta/math_type_traits.hpp"
#include <cassert>

namespace fetch {
namespace math {

template <typename ArrayType>
fetch::math::meta::IfIsNonBlasArray<ArrayType, void> Tanh(ArrayType &x)
{
  for (typename ArrayType::Type &e : x)
  {
    e = std::tanh(e);
  }
}
template <typename ArrayType>
fetch::math::meta::IfIsMathFixedPointArray<ArrayType, void> Tanh(ArrayType &x)
{
  for (typename ArrayType::Type &e : x)
  {
    e = static_cast<typename ArrayType::Type>(std::tanh(double(e)));
  }
}

}  // namespace math
}  // namespace fetch