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

#include "math/standard_functions/sqrt.hpp"
#include "vm/module.hpp"

#include <cmath>

namespace fetch {
namespace vm_modules {

inline void BindSqrt(vm::Module &module)
{
  module.CreateFreeFunction<float_t>("sqrt", &fetch::math::Sqrt<float_t>);
  module.CreateFreeFunction<double_t>("sqrt", &fetch::math::Sqrt<double_t>);
  module.CreateFreeFunction<fixed_point::fp32_t>("sqrt", &fetch::math::Sqrt<fixed_point::fp32_t>);
  module.CreateFreeFunction<fixed_point::fp64_t>("sqrt", &fetch::math::Sqrt<fixed_point::fp64_t>);
}

}  // namespace vm_modules
}  // namespace fetch
