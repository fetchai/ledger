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
#include "math/standard_functions/exp.hpp"
#include "vm/module.hpp"
#include "vm_modules/math/exp.hpp"

#include <cmath>

using namespace fetch::vm;

namespace fetch {
namespace vm_modules {
namespace math {

namespace {

template <typename T>
fetch::math::meta::IfIsMath<T, T> Exp(VM * /*vm*/, T const &a)
{
  T x;
  fetch::math::Exp(a, x);
  return x;
}

}  // namespace

void BindExp(Module &module)
{
  module.CreateFreeFunction("exp", &Exp<float_t>);
  module.CreateFreeFunction("exp", &Exp<double_t>);
  module.CreateFreeFunction("exp", &Exp<fixed_point::fp32_t>);
  module.CreateFreeFunction("exp", &Exp<fixed_point::fp64_t>);
}

}  // namespace math
}  // namespace vm_modules
}  // namespace fetch
