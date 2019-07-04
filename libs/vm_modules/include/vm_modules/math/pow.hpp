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

#include "math/meta/math_type_traits.hpp"
#include "math/standard_functions/pow.hpp"
#include "vm/module.hpp"

#include <cmath>

namespace fetch {
namespace vm_modules {
namespace math {

template <typename T>
fetch::math::meta::IfIsMath<T, T> Pow(fetch::vm::VM *, T const &a, T const &b)
{
  T x;
  fetch::math::Pow(a, b, x);
  return x;
}

inline void BindPow(fetch::vm::Module &module)
{
  module.CreateFreeFunction<float_t>("pow", &Pow<float_t>);
  module.CreateFreeFunction<double_t>("pow", &Pow<double_t>);
  module.CreateFreeFunction<fixed_point::fp32_t>("pow", &Pow<fixed_point::fp32_t>);
  module.CreateFreeFunction<fixed_point::fp64_t>("pow", &Pow<fixed_point::fp64_t>);
}

}  // namespace math
}  // namespace vm_modules
}  // namespace fetch
